from routing_api import RoutingAPI
from recordclass import recordclass

GraphEntry = recordclass("GraphEntry", "seq_num adj_node_dict")

# should be max connections/max connections-1
# controls how well the reverse edge algorithm works
SPARSITY = 3

class Routing(RoutingAPI):
    def __init__(self, wifi):
        super(Routing, self).__init__(wifi)

        # mapping from from mac to (seq_num, neighbor_node_dict)
        # each neighbor node is (to mac, connection type (0 if from is AP and to is STA, 1 else))
        self.graph = {}
        self.seq_num = 1

        self.enqued_msg = None

    def get_node_in_degree(self, mac):
        in_degree = 0
        for entry in self.graph[mac].adj_node_dict:
            if self.graph[mac].adj_node_dict[entry] == 0:
                in_degree += 1
        return in_degree

    def has_conntype_1(self, mac):
        for entry in self.graph[mac].adj_node_dict:
            if self.graph[mac].adj_node_dict[entry] == 1:
                return True
        return False

    # find closest node without a 1 connection
    def find_reversable_path(self):
        # tuple of (node,path)
        queue = []
        visited = {}
        queue.append((self.mac, [self.mac]))
        visited[self.mac] = True

        while queue:
            (node, path) = queue.pop(0)
            if not self.has_conntype_1(node):
                return path
            for mac in self.graph[node].adj_node_dict:
                if mac in self.graph and mac not in visited:
                    queue.append((mac, path+[mac]))
                    visited[mac] = True
        return None

    def tick_topology_algo(self):
        vis_macs = self.wifi.get_visible_macs()

        # if not connected to AP, search for one to connect to
        if self.wifi.get_connected_ap() == None:
            max_stren = 0
            max_mac = None
            for (mac, stren) in vis_macs:
                # filter out all the aps that are already connected to our AP
                if mac not in self.get_direct_conns():
                    if stren > max_stren:
                        max_stren = stren
                        max_mac = mac
            if max_mac:
                self.wifi.prt("Found ap and connecting: {}".format(max_mac))
                self.wifi.connect_to_ap(max_mac)
        
        # topology algorithms

        # reverse edge algorithm (run before sparse connection since fully connected mesh is more important than sparsity)
        # only check for reverse edge if you are connected to an AP
        # see if there is a visible mac not in the mesh
        # try and find a reversable path in your mesh
        # if found, reverse the path
        if self.wifi.get_connected_ap() and self.wifi.get_connected_ap() in self.graph:
            # find visible APs not in the mesh
            max_stren = 0
            max_mac = None
            for (mac, stren) in vis_macs:
                if mac not in self.graph:
                    if stren > max_stren:
                        max_mac = mac
                        max_stren = stren
            if max_mac:
                path = self.find_reversable_path()
                # self.wifi.prt(path)
                # send reverse edge
                self.send_reverse_edge(path)
                self.wifi.disconnect_from_ap()
                self.wifi.connect_to_ap(max_mac)

        # sparse connection algorithm
        # if I see a AP in the mesh with an in degree at least 2 smaller than
        # the AP I am connected to disconnect from current AP and connect to other AP
        if self.wifi.get_connected_ap() and self.wifi.get_connected_ap() in self.graph:
            # self.wifi.prt("graph after proc: {}".format(self.graph))
            cur_ap_in_degree = self.get_node_in_degree(self.wifi.get_connected_ap())

            min_in_degree = cur_ap_in_degree
            min_in_degree_mac = None
            for node in self.graph:
                if self.get_node_in_degree(node) < min_in_degree:
                    min_in_degree = self.get_node_in_degree(node)
                    min_in_degree_mac = node
        
            # threashold is SPARSITY
            if min_in_degree <= cur_ap_in_degree-SPARSITY:
                # self.wifi.prt("Sparsifying graph from {} to {}".format(self.wifi.get_connected_ap(), min_in_degree_mac))
                self.wifi.disconnect_from_ap()
                self.wifi.connect_to_ap(min_in_degree_mac)

        # no cycle algorithm
        # if there is a cycle in the graph have the STA with the smallest mac
        # disconnect from the AP
        # TODO

        # no duplicate connection algorithm
        # if we are connected to an ap that is also directly connected to us
        # disconnect if we have the lower mac
        # TODO

    # called when a station disconnects from my access point
    def mac_disconnected(self, mac):
        self.seq_num += 1
        # remove edge
        self.remove_edge(self.mac, mac)
        # send flood to network
        self.send_flood(None, self.mac, self.seq_num, 1, self.mac, mac)

    # called when a station connects to my access point
    def sta_connected(self, sta_mac):
        # add edges to graph
        self.add_edge(self.mac, sta_mac)
        # send bootstrap message
        self.send_bootstrap(sta_mac)
        # send flood to network
        # self.seq_num += 1
        # self.send_flood(None, self.mac, self.seq_num, 0, self.mac, sta_mac)

    # return all the nodes that this node knows of in the mesh
    def get_nodes(self):
        return list(self.graph.keys())

    # if no path to dest_mac, drop packet
    def send_message(self, dest_mac, msg):
        next_hop = self.find_next_hop(dest_mac)
        if next_hop:
            self.seq_num += 1
            self.send_directed(next_hop, self.mac, self.seq_num, dest_mac, msg)

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
        # Bootstrap packet (get graph)
        elif packets[0] == "B":
            self.on_receive_b(prev_mac, packets[1])
        # Reverse edge
        elif packets[0] == "R":
            self.on_receive_reverse_edge(packets[1])

        # self.wifi.prt("graph after proc: {}".format(self.graph))

    def get_direct_conns(self):
        return self.wifi.get_direct_connections()

    # create node if not yet initalized
    def create_node(self, mac):
        if mac not in self.graph:
            self.graph[mac] = GraphEntry(seq_num=0, adj_node_dict={})

    def add_edge(self, mac_ap, mac_sta):
        self.create_node(mac_ap)
        self.create_node(mac_sta)
        self.graph[mac_ap].adj_node_dict[mac_sta] = 0
        self.graph[mac_sta].adj_node_dict[mac_ap] = 1

    def prune_graph(self):
        disconnected_nodes = []
        for node in self.graph:
            disconnected_nodes.append(node)

        queue = []
        visited = {}
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


    def remove_edge(self, mac_1, mac_2):
        # remove 2 from 1
        if mac_1 in self.graph and mac_2 in self.graph[mac_1].adj_node_dict:
            del self.graph[mac_1].adj_node_dict[mac_2]
        # remove 1 from 2
        if mac_2 in self.graph and mac_1 in self.graph[mac_2].adj_node_dict:
            del self.graph[mac_2].adj_node_dict[mac_1]

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

    # return all edges with conn_type == 0
    def get_ap_connections(self):
        conns = []
        for node in self.graph:
            for entry in self.graph[node].adj_node_dict:
                if self.graph[node].adj_node_dict[entry] == 0:
                    conns.append((node, entry))

        return conns


    def on_receive_b(self, prev_mac, data):
        # bootstrap packets are as follows "from_mac|seq_num|to_mac1|conn_type1|...|\nfrom_mac"
        nodes = data.split("\n")
        for n in nodes:
            node_data = n.split("|")
            if len(node_data) >= 2:
                from_mac = node_data[0]
                from_seq_num = node_data[1]
                for i in range(2,len(node_data)-1,2):
                    to_mac = node_data[i]
                    conn_type = int(node_data[i+1])
                    # avoid duplicates by only adding conn_type 0
                    if conn_type == 0:
                        self.add_edge(from_mac, to_mac)

        # send flood messages of edges not known to either component
        known_connections = self.get_ap_connections()
        # self.wifi.prt(known_connections)
        # flood connections
        for (ap_mac, sta_mac) in known_connections:
            self.seq_num += 1
            self.send_flood(None, self.mac, self.seq_num, 0, ap_mac, sta_mac)

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
                self.add_edge(mac_1, mac_2)
            elif f_type == 1:
                # remove edge
                self.remove_edge(mac_1, mac_2)
            self.send_flood(prev_mac, orig_mac, orig_seq_num, f_type, mac_1, mac_2)

    def on_receive_reverse_edge(self, data):
        path = data.split("|")
        # self.wifi.prt("recved rev edge: {}".format(path))
        if path[1] == self.mac:
            rev_ap = path[0]
            path.pop(0)
            # send reverse if not last node
            if len(path) > 1:
                self.send_reverse_edge(path)
                # only need to disconnect if not last node
                self.wifi.disconnect_from_ap()
            
            # connect to new ap
            self.wifi.connect_to_ap(rev_ap)


    # dest should be a neighbor
    def send_data(self, dest_mac, msg):
        if not self.wifi.send_data(dest_mac, msg):
            # if send fails, flood network with broken edge from self to dest
            self.mac_disconnected(dest_mac)

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

    def send_reverse_edge(self, path):
        payload = "|".join(path)
        dest = path[1]
        self.send_data(dest, "R,{}".format(payload))