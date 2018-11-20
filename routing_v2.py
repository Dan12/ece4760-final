from routing_api import RoutingAPI, GraphEntry

class Routing(RoutingAPI):
    def __init__(self, wifi):
        super(Routing, self).__init__(wifi)

        self.seq_num = 1

    def prt(self, msg):
        print("Router {}: {}".format(self.mac, msg))

    # BEGIN WIFI PASSTHROUGH
    def connect_to_ap(self, ap_mac):
        self.wifi.connect_to_ap(ap_mac)

    def get_direct_connections(self):
        self.wifi.get_direct_connections()

    def disconnect_from_ap(self):
        # self.mac_disconnected(self.get_connected_ap())
        self.wifi.disconnect_from_ap()

    def get_connected_ap(self):
        return self.wifi.get_connected_ap()

    def get_visible_macs(self):
        return self.wifi.get_visible_macs()
    # END WIFI PASSTHROUGH

    # called when a station disconnects from my access point
    def mac_disconnected(self, mac):
        # Could be a duplicate connection disconnection
        if mac not in self.get_direct_conns():
            if mac in self.graph:
                self.seq_num = max(self.seq_num, self.graph[mac].seq_num) + 1
            else:
                self.seq_num += 1
            # remove edge
            self.remove_edge(self.mac, mac, self.seq_num)
            # send flood to network
            self.send_flood([], self.mac, self.seq_num, 1, self.mac, mac)

    # called when a station connects to my access point
    def sta_connected(self, sta_mac):
        self.seq_num += 1
        # send bootstrap message
        self.send_bootstrap(sta_mac)

    # return the routing graph
    def get_graph(self):
        return self.graph

    # if no path to dest_mac, drop packet
    def send_message(self, dest_mac, msg):
        # self.prt("Sending to {}: {}".format(dest_mac, msg))
        next_hop = self.find_next_hop(dest_mac)
        if next_hop:
            self.seq_num += 1
            self.send_directed(next_hop, self.mac, self.seq_num, dest_mac, msg)
        else:
            self.prt("WARNING: no next hop found for {}".format(dest_mac))

    def recv_message(self, prev_mac, msg):
        packets = msg.split(",")
        if len(packets) < 1:
            return

        # DYNAMIC ROUTE DISCOVERY
        # Flood packet (orig,seq_num,type,mac1,mac2) mac1 is AP, 0 type is create edge, 1 type is remove edge
        if packets[0] == "F":
            self.on_receive_f(prev_mac, packets[1], int(packets[2]), int(packets[3]), packets[4], packets[5])
        # Directed packet (orig,seq_num,dest,msg)
        elif packets[0] == "D":
            self.on_receive_d(prev_mac, packets[1], int(packets[2]), packets[3], packets[4])
        # Bootstrap packet (seq_num,graph)
        elif packets[0] == "B":
            self.on_receive_b(prev_mac, int(packets[1]), packets[2])

        # self.prt("graph after proc: {}".format(self.graph))

    def get_direct_conns(self):
        return self.wifi.get_direct_connections()

    # create node if not yet initalized
    def create_node(self, mac):
        if mac not in self.graph:
            self.graph[mac] = GraphEntry(seq_num=0, adj_node_dict={})

    def add_edge(self, mac_ap, mac_sta, seq_num):
        self.create_node(mac_ap)
        self.create_node(mac_sta)
        if max(self.graph[mac_ap].seq_num, self.graph[mac_sta].seq_num) <= seq_num:
            self.graph[mac_ap].adj_node_dict[mac_sta] = 0
            self.graph[mac_sta].adj_node_dict[mac_ap] = 1
            # update sequence numbers
            self.graph[mac_ap].seq_num = seq_num
            self.graph[mac_sta].seq_num = seq_num
            # potentially update own seq num
            if mac_ap == self.mac or mac_sta == self.mac:
                self.seq_num = max(self.seq_num, seq_num)

    def prune_graph(self):
        disconnected_nodes = []
        for node in self.graph:
            disconnected_nodes.append(node)

        queue = []
        visited = {}
        if self.mac in self.graph:
            queue.append(self.mac)
            visited[self.mac] = True
        while queue:
            node = queue.pop(0)
            disconnected_nodes.remove(node)
            for mac in self.graph[node].adj_node_dict:
                if mac in self.graph and mac not in visited:
                    queue.append(mac)
                    visited[mac] = True

        for node in disconnected_nodes:
            del self.graph[node]


    def remove_edge(self, mac_1, mac_2, seq_num):
        if mac_1 in self.graph and mac_2 in self.graph:
            if max(self.graph[mac_1].seq_num, self.graph[mac_2].seq_num) <= seq_num:
                del self.graph[mac_1].adj_node_dict[mac_2]
                del self.graph[mac_2].adj_node_dict[mac_1]
                # update sequence numbers
                self.graph[mac_1].seq_num = seq_num
                self.graph[mac_2].seq_num = seq_num
                # potentially update own seq num
                if mac_1 == self.mac or mac_2 == self.mac:
                    self.seq_num = max(self.seq_num, seq_num)

        # prune graph if we now have 2 components
        self.prune_graph()


    def is_valid_seq_num(self, mac, seq_num):
        if mac in self.graph:
            # only valid if new seq number is greater than most recently seen
            if self.graph[mac].seq_num < seq_num:
                # update most recently seen seq num
                self.graph[mac].seq_num = seq_num
                return True
            else:
                return False
        return True

    def find_next_hop(self, dest_mac):
        # tuple of (node,init_mac)
        queue = []
        visited = {}
        visited[self.mac] = True
        for mac in self.get_direct_conns():
            if mac in self.graph:
                queue.append((mac, mac))
                visited[mac] = True
        while queue:
            (node, init_hop) = queue.pop(0)
            if node == dest_mac:
                return init_hop
            for mac in self.graph[node].adj_node_dict:
                if mac in self.graph and mac not in visited:
                    queue.append((mac, init_hop))
                    visited[mac] = True
        return None

    # return all edges with conn_type == 1 (station->ap)
    def get_ap_connections(self):
        conns = []
        for node in self.graph:
            for entry in self.graph[node].adj_node_dict:
                if self.graph[node].adj_node_dict[entry] == 1:
                    conns.append((node, self.graph[node].seq_num, entry))

        return conns

    def on_receive_b(self, prev_mac, seq_num, data):
        # get my connections before adding new ones
        my_side = self.get_ap_connections()

        other_side = []
        # bootstrap packets are as follows "from_mac|seq_num|to_mac1\n..."
        nodes = data.split("\n")
        for n in nodes:
            node_data = n.split("|")
            if len(node_data) >= 3:
                # station
                from_mac = node_data[0]
                from_seq_num = int(node_data[1])
                # ap
                to_mac = node_data[2]

                self.add_edge(to_mac, from_mac, from_seq_num)
                # prune for cycles?
                # TODO also prune where edge is same but seq num is different
                if (from_mac, from_seq_num, to_mac) in my_side:
                    my_side.remove((from_mac, from_seq_num, to_mac))
                else:
                    other_side.append((from_mac, from_seq_num, to_mac))
                # other_side.append((from_mac, from_seq_num, to_mac))

        # self.prt(my_side)
        # self.prt(other_side)

        my_side_macs = self.get_direct_conns()
        my_side_macs.remove(prev_mac)
        # replay edge connections from my side to the other side
        for (sta_mac, sta_seq_num, ap_mac) in my_side:
            self.send_flood(my_side_macs, sta_mac, sta_seq_num, 0, ap_mac, sta_mac)
        # replay edge connections from other side to my side
        for (sta_mac, sta_seq_num, ap_mac) in other_side:
            self.send_flood(prev_mac, sta_mac, sta_seq_num, 0, ap_mac, sta_mac)

        # finally, broadcast my new edge
        self.seq_num = max(self.seq_num, seq_num) + 1
        self.add_edge(prev_mac, self.mac, self.seq_num)
        # send message to create the edge (I am the station)
        self.send_flood([], self.mac, self.seq_num, 0, prev_mac, self.mac)

    def on_receive_d(self, prev_mac, orig_mac, orig_seq_num, dest_mac, msg):
        if self.is_valid_seq_num(orig_mac, orig_seq_num):
            if dest_mac == self.mac:
                # call the handler
                self.on_recv_msg_handler(orig_mac, msg)
            else:
                next_hop = self.find_next_hop(dest_mac)
                self.send_directed(next_hop, orig_mac, orig_seq_num, dest_mac, msg)

    def on_receive_f(self, prev_mac, orig_mac, orig_seq_num, f_type, mac_1, mac_2):
        if self.is_valid_seq_num(orig_mac, orig_seq_num):
            if f_type == 0:
                # create edge
                self.add_edge(mac_1, mac_2, orig_seq_num)
            elif f_type == 1:
                # remove edge
                self.remove_edge(mac_1, mac_2, orig_seq_num)
            self.send_flood([prev_mac], orig_mac, orig_seq_num, f_type, mac_1, mac_2)

    # dest should be a neighbor
    def send_data(self, dest_mac, msg):
        if not self.wifi.send_data(dest_mac, msg):
            # if send fails, flood network with broken edge from self to dest
            self.mac_disconnected(dest_mac)

    def send_bootstrap(self, bootstrap_mac):
        payload = ""
        for (sta_mac, sta_seq_num, ap_mac) in self.get_ap_connections():
            payload += "{}|{}|{}\n".format(sta_mac, sta_seq_num, ap_mac)

        self.send_data(bootstrap_mac, "B,{},{}".format(self.seq_num, payload))

    def send_directed(self, next_mac, orig_mac, orig_seq_num, dest_mac, msg):
        self.send_data(next_mac, "D,{},{},{},{}".format(orig_mac, orig_seq_num, dest_mac, msg))

    def send_flood(self, exclude_macs, orig_mac, orig_seq_num, f_type, mac_1, mac_2):
        for neighbor_mac in self.get_direct_conns():
            if neighbor_mac not in exclude_macs:
                self.send_data(neighbor_mac, "F,{},{},{},{},{}".format(orig_mac, orig_seq_num, f_type, mac_1, mac_2))