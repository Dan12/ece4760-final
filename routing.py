from collections import namedtuple

class RouteTableEntry(namedtuple("RouteTableEntry", ["dst_mac", "dst_seq_num", "v_dst_seq_num", "state", "hop_count", "next_hop"])):
    __slots__ = ()
    def __str__(self):
        pass

class RREQMessage(namedtuple("RREQMessage", ["hop_count", "rreq_id", "dst_mac", "dst_seq_num", "u_dst_seq_num", "orig_mac", "orig_seq_num"])):
    __slots__ = ()
    def __str__(self):
        "{},{},{},{},{},{},{}".format(self.hop_count, self.rreq_id, self.dst_mac, self.dst_seq_num, self.u_dst_seq_num, self.orig_mac, self.orig_seq_num)

class RREPMessage(namedtuple("RREPMessage", ["hop_count", "dst_mac", "dst_seq_num", "orig_mac"])):
    __slots__ = ()
    def __str__(self):
        "{},{},{},{}".format(self.hop_count, self.dst_mac, self.dst_seq_num, self.orig_mac)

class RoutingLayer:
    def __init__(self, wifi):
        # mapping from MAC addresses to route table entry
        self.route_table = {}
        # mapping from orig MAC addresses to highest rreq id seen for that address
        self.rreq_id_table = {}
        self.rreq_id = 1
        self.seq_num = 1
        self.mac = wifi.mac
        self.enqued_msg = None
        self.wifi = wifi

    def update_table_entry(self, entry_mac, )

    # 0 -> sent message
    # 1 -> message enqueued, wait until done
    # 2 -> no route yet, sending rreq and enqueued message
    def create_msg(self, dest_mac, msg):
        # cant send a message while enqued
        # if self.enqued_msg:
        #     return 1

        # next_hop = self.get_next_hop(dest_mac)
        # if next_hop:
        #     if self.send_msg(next_hop, self.mac, dest_mac, 0, msg):
        #         return 0

        # # if sending message failed, send rreq for dest
        # self.enqued_msg = (dest_mac, msg)
        # for dir_mac in self.get_direct_conns():
        #     self.send_rreq(dir_mac, self.mac, dest_mac, 0, self.msg_id)
        # # incement broadcast ID
        # self.msg_id += 1
        # return 2
        pass

    def handle_recv_msg(self, src_mac, msg):
        self.wifi.prt("msg from {}: {}".format(src_mac, msg))

    def get_direct_conns(self):
        return self.wifi.get_direct_macs()

    def send_data(self, dest_mac, msg):
        data_succ = self.wifi.send_data_to_mac(dest_mac, msg)
        if data_succ:
            return True

        # TODO invalidate route to dest?
        # self.invalidate_table(dest_mac)        
        return False

    def create_rreq(self, dst_mac):
        self.rreq_id += 1
        self.seq_num += 1
        rreq_msg = RREQMessage(
            hop_count=0, 
            rreq_id=self.rreq_id, 
            dst_mac=dst_mac, 
            dst_seq_num=0, 
            u_dst_seq_num=0, 
            orig_mac=self.mac, 
            orig_seq_num=self.seq_num)
        if dst_mac in self.route_table:
            # old but invalid seq number
            rreq_msg.dst_seq_num = self.route_table[dst_mac].dst_seq_num
        else:
            # set unknown seq number to 1 (true)
            rreq_msg.u_dst_seq_num = 1

        for dir_mac in self.get_direct_conns():
            # send the rreq to all neighbors
            self.send_rreq(rreq_msg)

    def process_data(self, prev_mac, packets):
        # DYNAMIC ROUTE DISCOVERY
        # Flood packet (RREQ)
        if packets[0] == "F":
            rreq_msg = RREQMessage(
                hop_count=int(packets[1]), 
                rreq_id=int(packets[2]),
                dst_mac=packets[6],
                dst_seq_num=int(packets[4]),
                u_dst_seq_num=int(packets[5]),
                orig_mac=packets[6], 
                orig_seq_num=int(packets[7]))
            self.on_receive_rreq(prev_mac, rreq_msg)
        # Flood receive packet (RREP)
        elif packets[0] == "FR":
            self.on_receive_rrep(prev_mac, packets[1], packets[2], int(packets[3]))
        # Message packet (src,dest,hop_count,msg_id,data)
        elif packets[0] == "M":
            pass
            # self.on_receive_msg(prev_mac, packets[1], packets[2], int(packets[3]), int(packets[4]), packets[5])
        # Message Error (E,orig,dst)
        elif packets[0] == "E":
            pass
            # self.on_receive_error(prev_mac, packets[1], packets[2])

        self.wifi.prt("routes after proc: {}".format(self.routes))

    def on_receive_rreq(self, prev_mac, rreq_msg):
        # update table for reverse route
        self.update_table(prev_mac, prev_mac, 1)

    def on_receive_rrep(self, prev_mac, orig_mac, dest_mac, hop_count):
        pass

    def on_receive_msg(self, prev_mac, orig_mac, dest_mac, hop_count, msg_id, msg):
        pass    

    # handle error from sending message from
    def on_receive_error(self, prev_mac, orig_mac, dest_mac):
        pass        

    def send_rreq(self, next_mac, orig_mac, dest_mac, hop_count, msg_id):
        return self.send_data(next_mac, "F,{},{},{},{}".format(orig_mac, dest_mac, hop_count, msg_id))

    def send_rrep(self, next_mac, orig_mac, dest_mac, hop_count):
        return self.send_data(next_mac, "FR,{},{},{}".format(orig_mac, dest_mac, hop_count))

    def send_msg(self, next_mac, orig_mac, dest_mac, hop_count, msg):
        pass
        # mid = self.msg_id
        # self.msg_id += 1
        # return self.send_data(next_mac, "M,{},{},{},{},{}".format(orig_mac, dest_mac, hop_count, mid, msg))

    def send_error(self, next_mac, orig_mac, dest_mac):
        pass
        # return self.send_data(next_mac, "E,{},{}".format(orig_mac, dest_mac))