from topology_api import TopologyAPI

SPARSITY = 3

class Topology(TopologyAPI):

    Sims = []

    @staticmethod
    def simulate():
        for t in Topology.Sims:
            t.run()

    def __init__(self, router):
        super(Topology, self).__init__(router)
        Topology.Sims.append(self)

    def prt(self, msg):
        print("Topo {}: {}".format(self.mac, msg))

    # called when a message is passed from routing to topology layer
    def recv_msg(self, from_mac, msg):
        packets = msg.split(",")
        # Message to next layer
        if packets[0] == "M":
            self.on_recv_msg_handler(from_mac, ",".join(packets[1:]))
        # reverse edge packet
        elif packets[0] == "R":
            self.on_receive_reverse_edge(from_mac, ",".join(packets[1:]))

    def send_msg(self, dest_mac, msg):
        self.router.send_message(dest_mac, "M,{}".format(msg))

    def get_nodes(self):
        return self.router.get_graph().keys()

    def on_receive_reverse_edge(self, from_mac, path_msg):
        path = path_msg.split(",")
        if path[1] == self.mac:
            rev_ap = path[0]
            path.pop(0)
            # send reverse if not last node
            if len(path) > 1:
                self.send_reverse_edge(path)
                # only need to disconnect if not last node
                self.router.disconnect_from_ap()
            
            # connect to new ap
            self.router.connect_to_ap(rev_ap)

    def send_reverse_edge(self, path):
        payload = ",".join(path)
        dest = path[1]
        self.router.send_message(dest, "R,{}".format(payload))

    def get_node_in_degree(self, graph, mac):
        in_degree = 0
        for entry in graph[mac].adj_node_dict:
            if graph[mac].adj_node_dict[entry] == 0:
                in_degree += 1
        return in_degree

    def is_in_cycle(self, mac, visited):
        if mac not in visited:
            visited[mac] = True
            if mac in self.router.get_graph():
                for entry in self.router.get_graph()[mac].adj_node_dict:
                    # only follow 1 edges (i.e. direction)
                    if self.router.get_graph()[mac].adj_node_dict[entry] == 1:
                        (cycle, min_mac) = self.is_in_cycle(entry, visited)
                        if cycle:
                            return (True, min(min_mac, mac))
        elif mac == self.mac:
            return (True, self.mac)
        return (False, -1)

    def has_conntype_1(self, graph, mac):
        for entry in graph[mac].adj_node_dict:
            if graph[mac].adj_node_dict[entry] == 1:
                return True
        return False

    # find closest node without a 1 connection
    def find_reversable_path(self, graph):
        # tuple of (node,path)
        queue = []
        visited = {}
        queue.append((self.mac, [self.mac]))
        visited[self.mac] = True

        while queue:
            (node, path) = queue.pop(0)
            if not self.has_conntype_1(graph, node):
                return path
            for mac in graph[node].adj_node_dict:
                if mac in graph and mac not in visited:
                    queue.append((mac, path+[mac]))
                    visited[mac] = True
        return None

    # run the topology algorithms
    def run(self):
        vis_mac_strns = self.router.get_visible_macs()

        graph = self.router.get_graph()
        connected_ap = self.router.get_connected_ap()

        # if not connected to AP, search for one to connect to
        if connected_ap == None:
            max_stren = 0
            max_mac = None
            print(self.router.get_direct_conns())
            print(graph)
            print(vis_mac_strns)
            for (mac, stren) in vis_mac_strns:
                # filter out all the aps that are already connected to our AP and that are in our graph
                if mac not in self.router.get_direct_conns() and mac not in graph:
                    if stren > max_stren:
                        max_stren = stren
                        max_mac = mac
            if max_mac:
                self.prt("Found ap and connecting: {}".format(max_mac))
                self.router.connect_to_ap(max_mac)
                return

        # reverse edge algorithm (run before sparse connection since fully connected mesh is more important than sparsity)
        # only check for reverse edge if you are connected to an AP
        # see if there is a visible mac not in the mesh
        # try and find a reversable path in your mesh
        # if found, reverse the path
        if connected_ap and connected_ap in graph:
            # find visible APs not in the mesh
            max_stren = 0
            max_mac = None
            for (mac, stren) in vis_mac_strns:
                if mac not in graph:
                    if stren > max_stren:
                        max_mac = mac
                        max_stren = stren
            if max_mac:
                path = self.find_reversable_path(graph)
                # self.prt(path)
                # send reverse edge
                self.send_reverse_edge(path)
                self.router.disconnect_from_ap()
                self.router.connect_to_ap(max_mac)
                return

        # sparse connection algorithm
        # if I see a AP in the mesh with an in degree at least 2 smaller than
        # the AP I am connected to disconnect from current AP and connect to other AP
        if connected_ap and connected_ap in graph:
            cur_ap_in_degree = self.get_node_in_degree(graph, connected_ap)

            min_in_degree = cur_ap_in_degree
            min_in_degree_mac = None
            for (mac, stren) in vis_mac_strns:
                if mac in graph and self.get_node_in_degree(graph, mac) < min_in_degree:
                    min_in_degree = self.get_node_in_degree(graph, mac)
                    min_in_degree_mac = mac
        
            # threashold is SPARSITY
            if min_in_degree <= cur_ap_in_degree-SPARSITY:
                self.prt("Sparsifying graph from {} to {}".format(self.router.get_connected_ap(), min_in_degree_mac))
                self.router.disconnect_from_ap()
                self.router.connect_to_ap(min_in_degree_mac)
                return

        # no cycle property
        # if there is a direct cycle in the graph and I am the smallest node in that cycle
        # disconnect from my ap
        (cycle, min_mac) = self.is_in_cycle(self.mac, {})
        if cycle and min_mac == self.mac:
            self.router.disconnect_from_ap()
            return