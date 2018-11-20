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
        packets = msg.split("|")
        # Message to next layer
        if packets[0] == "M":
            self.on_recv_msg_handler(from_mac, packets[1])
        # reverse edge packet
        elif packets[0] == "R":
            self.on_receive_reverse_edge(from_mac, packets[1])

    def send_msg(self, dest_mac, msg):
        self.router.send_message(dest_mac, "M|{}".format(msg))

    def get_nodes(self):
        return self.router.get_graph().keys()

    def on_receive_reverse_edge(self, from_mac, path_msg):
        pass

    def get_node_in_degree(self, graph, mac):
        in_degree = 0
        for entry in graph[mac].adj_node_dict:
            if graph[mac].adj_node_dict[entry] == 0:
                in_degree += 1
        return in_degree

    # run the topology algorithms
    def run(self):
        vis_mac_strns = self.router.get_visible_macs()

        graph = self.router.get_graph()
        connected_ap = self.router.get_connected_ap()

        # if not connected to AP, search for one to connect to
        if connected_ap == None:
            max_stren = 0
            max_mac = None
            for (mac, stren) in vis_mac_strns:
                # filter out all the aps that are already connected to our AP
                if mac not in self.router.get_direct_conns():
                    if stren > max_stren:
                        max_stren = stren
                        max_mac = mac
            if max_mac:
                self.prt("Found ap and connecting: {}".format(max_mac))
                self.router.connect_to_ap(max_mac)

        # no duplicate connection algorithm
        # if we are connected to an ap that is also directly connected to us
        # disconnect if we have the lower mac
        if connected_ap and connected_ap in graph:
            # either our connected ap has a 1 edge to us or we have a 0 edge to our AP
            if graph[connected_ap].adj_node_dict[self.mac] == 1:
                # 1 edge where there should be 0
                self.prt("1 edge where there should be 0")
                if self.mac < connected_ap:
                    # Prefer me to be the AP, disconnect my station
                    self.router.disconnect_from_ap()
                else:
                    # correct the edge by saying I am the station and other is the AP
                    self.router.add_graph_edge(connected_ap, self.mac)
            elif graph[self.mac].adj_node_dict[connected_ap] == 0:
                # 0 edge where there should be 1
                self.prt("0 edge where there should be 1")
                if self.mac < connected_ap:
                    # Prefer me to be the AP, disconnect my station
                    self.router.disconnect_from_ap()
                else:
                    # correct the edge by saying I am the station and other is the AP
                    self.router.add_graph_edge(connected_ap, self.mac)

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
            if min_in_degree <= cur_ap_in_degree-SPARSITY and self.mac < min_in_degree_mac:
                self.prt("Sparsifying graph from {} to {}".format(self.router.get_connected_ap(), min_in_degree_mac))
                self.router.disconnect_from_ap()
                self.router.connect_to_ap(min_in_degree_mac)

        # self.prt("graph after proc: {}".format(self.router.get_graph()))