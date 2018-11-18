from recordclass import recordclass

GraphEntry = recordclass("GraphEntry", "seq_num adj_node_dict")

class RoutingLayer:
    def __init__(self, wifi):
        # mapping from from mac to (seq_num, neighbor_node_dict)
        # each neighbor node is (to mac, connection type (0 if from is AP and to is STA, 1 else))
        self.graph = {}
        self.seq_num = 1
        self.mac = wifi.mac
        self.enqued_msg = None
        self.wifi = wifi

    def create_msg(self, dest_mac, msg):
        next_hop = self.find_next_hop(dest_mac)
        self.seq_num += 1
        self.send_directed(next_hop, self.mac, self.seq_num, dest_mac, msg)

    def handle_recv_msg(self, src_mac, msg):
        self.wifi.prt("msg from {}: {}".format(src_mac, msg))

    def get_direct_conns(self):
        return self.wifi.get_direct_macs()

    # dest should be a neighbor
    def send_data(self, dest_mac, msg):
        data_succ = self.wifi.send_data_to_mac(dest_mac, msg)
        if data_succ:
            return True

        return False

    # called by wifi when a connection is made
    def made_ap_connection(self, sta_mac):
        # add edges to graph
        self.add_edge(self.mac, sta_mac)
        # send bootstrap message
        self.send_bootstrap(sta_mac)
        # send flood to network
        self.seq_num += 1
        self.send_flood(None, self.mac, self.seq_num, 0, self.mac, sta_mac)

    def lost_connection(self, other_mac):
        conn_type = self.graph[self.mac].adj_node_dict[other_mac]

        self.seq_num += 1
        if conn_type == 0:
            # remove edge
            self.remove_edge(self.mac, other_mac)
            # send flood to network
            self.send_flood(None, self.mac, self.seq_num, 1, self.mac, other_mac)
        else:
            self.remove_edge(other_mac, self.mac)
            self.send_flood(None, self.mac, self.seq_num, 1, other_mac, self.mac)

    # create node if not yet initalized
    def create_node(self, mac):
        if mac not in self.graph:
            self.graph[mac] = GraphEntry(seq_num=0, adj_node_dict={})

    def add_edge(self, mac_ap, mac_sta):
        self.create_node(mac_ap)
        self.create_node(mac_sta)
        self.graph[mac_ap].adj_node_dict[mac_sta] = 0
        self.graph[mac_sta].adj_node_dict[mac_ap] = 1

    def remove_edge(self, mac_ap, mac_sta):
        # remove sta from ap node
        if mac_ap in self.graph and mac_sta in self.graph[mac_ap].adj_node_dict:
            del self.graph[mac_ap].adj_node_dict[mac_sta]
        # remove ap from sta node
        if mac_sta in self.graph and mac_ap in self.graph[mac_sta].adj_node_dict:
            del self.graph[mac_sta].adj_node_dict[mac_ap]

        # TODO prune graph if we now have 2 components

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

    def process_data(self, prev_mac, packets):
        # DYNAMIC ROUTE DISCOVERY
        # Flood packet (orig,seq_num,type,mac1,mac2) mac1 is AP, 0 type is create edge, 1 type is remove edge
        if packets[0] == "F":
            self.on_receive_f(prev_mac, packets[1], int(packets[2]), int(packets[3]), packets[4], packets[5])
        # Directed packet (orig,seq_num,dest,msg)
        elif packets[0] == "D":
            self.on_receive_d(prev_mac, packets[1], int(packets[2]), packets[3], packets[4])
        # Bootstrap packet (get graph)
        elif packets[0] == "B":
            self.on_receive_b(packets[1])

        # self.wifi.prt("graph after proc: {}".format(self.graph))

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

    def on_receive_b(self, data):
        # bootstrap packets are as follows "from_mac|seq_num|to_mac1|conn_type1|...|\nfrom_mac"
        nodes = data.split("\n")
        for n in nodes:
            node_data = n.split("|")
            if len(node_data) >= 2:
                from_mac = node_data[0]
                from_seq_num = node_data[1]
                for i in range(2,len(node_data)-1,2):
                    to_mac = node_data[i]
                    conn_type = node_data[i+1]
                    if conn_type == 0:
                        self.add_edge(from_mac, to_mac)
                    else:
                        self.add_edge(to_mac, from_mac)

    def on_receive_d(self, prev_mac, orig_mac, orig_seq_num, dest_mac, msg):
        if self.is_valid_seq_num(orig_mac, orig_seq_num):
            if dest_mac == self.mac:
                self.handle_recv_msg(orig_mac, msg)
            else:
                next_hop = self.find_next_hop(dest_mac)
                self.send_directed(next_hop, orig_mac, orig_seq_num, dest_mac, msg)

    def on_receive_f(self, prev_mac, orig_mac, orig_seq_num, f_type, mac_1, mac_2):
        if self.is_valid_seq_num(orig_mac, orig_seq_num):
            if f_type == 0:
                # create edge
                self.add_edge(mac_1, mac_2)
            elif f_type == 1:
                # remove edge
                self.remove_edge(mac_1, mac_2)
            self.send_flood(prev_mac, orig_mac, orig_seq_num, f_type, mac_1, mac_2)

    def send_bootstrap(self, bootstrap_mac):
        payload = ""
        for mac in self.graph:
            payload += "{}|{}|".format(mac, self.graph[mac].seq_num)
            for to_mac in self.graph[mac].adj_node_dict:
                payload += "{}|{}|".format(to_mac, self.graph[mac].adj_node_dict[to_mac])
            payload += "\n"

        self.send_data(bootstrap_mac, "B,{}".format(payload))

    def send_directed(self, next_mac, orig_mac, orig_seq_num, dest_mac, msg):
        self.send_data(next_mac, "D,{},{},{},{}".format(orig_mac, orig_seq_num, dest_mac, msg))

    def send_flood(self, prev_mac, orig_mac, orig_seq_num, f_type, mac_1, mac_2):
        for neighbor_mac in self.get_direct_conns():
            if neighbor_mac != prev_mac:
                self.send_data(neighbor_mac, "F,{},{},{},{},{}".format(orig_mac, orig_seq_num, f_type, mac_1, mac_2))