class RoutingLayer:
    def __init__(self, wifi):
        # mapping from MAC addresses to (next hop, hop count, broadcast_id)
        self.routes = {}
        self.broadcast_id = 1
        self.mac = wifi.mac
        self.enqued_msg = None
        self.wifi = wifi

    # 0 -> sent message
    # 1 -> message enqueued, wait until done
    # 2 -> no route yet, sending rreq and enqueued message
    def create_msg(self, dest_mac, msg):
        # cant send a message while enqued
        if self.enqued_msg:
            return 1

        next_hop = self.get_next_hop(dest_mac)
        if next_hop:
            self.send_msg(next_hop, self.mac, dest_mac, 0, msg)
            return 0
        else:
            self.enqued_msg = (dest_mac, msg)
            for dir_mac in self.get_direct_conns():
                self.send_rreq(dir_mac, self.mac, dest_mac, 0, self.broadcast_id)
            # incement broadcast ID
            self.broadcast_id += 1
            return 2

    def handle_recv_msg(self, src_mac, msg):
        print("msg from {}: {}".format(src_mac, msg))

    def process_data(self, prev_mac, packets):
        # DYNAMIC ROUTE DISCOVERY
        # Flood packet (RREQ) of form F,orig,dest,hop_cnt,id
        if packets[0] == "F":
            self.on_receive_rreq(prev_mac, packets[1], packets[2], packets[3], packets[4])
        # Flood receive packet (RREP) of form FR,orig,dest,hop_cnt
        elif packets[0] == "FR":
            self.on_receive_rrep(prev_mac, packets[1], packets[2], packets[3])
        # Message packet (src,dest,data)
        elif packets[0] == "M":
            self.on_receive_msg(prev_mac, packets[1], packets[2], packets[3])
        # Message Error (E,orig,dst)
        elif packets[0] == "E":
            self.on_receive_error(prev_mac, packets[1], packets[2])

    def get_direct_conns(self):
        return self.wifi.get_direct_macs()

    def send_data(self, dest_mac, msg):
        self.wifi.send_data_to_mac(dest_mac, msg)

    def get_broadcast_id(self, mac):
        if mac in self.routes:
            return self.routes[mac][2]
        else:
            return 0

	# check if duplicate broadcast id and update broadcast_id if not
    def is_duplicate_packet(self, mac, broadcast_id):
        if mac not in self.routes:
            return False
        if self.routes[mac][2] < broadcast_id:
            self.routes[mac] = (self.routes[mac][0], self.routes[mac][1], broadcast_id)
            return True
        else:
            return False

    def update_table(self, mac, next_hop, hop_count):
        if mac not in self.routes:
            self.routes[mac] = (next_hop, hop_count, 0)
        else:
            # if we have a better path, set it to that (maintain broadcast_id)
            if self.routes[mac][1] > hop_count:
                self.routes[mac] = (next_hop, hop_count, self.routes[mac][2])

    # if the mac we are trying to invalidate has the next hop equal to the
    # mac we got the error from, delete that table entry
    def invalidate_table(self, mac, prev_mac):
        if self.routes[mac]:
            if self.routes[mac][0] == prev_mac:
                del self.routes[mac]

    def get_next_hop(self, dest_mac):
        if dest_mac in self.routes:
            return self.routes[dest_mac][0]
        return None

    def get_hop_count(self, dest_mac):
        if dest_mac in self.routes:
            return self.routes[dest_mac][1]
        return 0

    def on_receive_rreq(self, prev_mac, orig_mac, dest_mac, hop_count, broadcast_id):
        # update the reverse route to the orig
        self.update_table(orig_mac, prev_mac, hop_count)
        # check if this is a duplicate rreq
        if not self.is_duplicate_packet(orig_mac, broadcast_id):
            # update hop count
            hop_count+=1
            if self.routes[dest_mac]:
                next_hop = self.get_next_hop(orig_mac)
                # TODO if no next hop, drop packet
                self.send_rrep(next_hop, orig_mac, dest_mac, self.get_hop_count(dest_mac))
            elif self.mac == dest_mac:
                next_hop = self.get_next_hop(orig_mac)
                # TODO if no next hop, drop packet
                self.send_rrep(next_hop, orig_mac, dest_mac, 0)
            else:
                for dir_mac in self.get_direct_conns():
                    self.send_rreq(dir_mac, orig_mac, dest_mac, hop_count, broadcast_id)

    def on_receive_rrep(self, prev_mac, orig_mac, dest_mac, hop_count):
        # update the table entry to the destination
        self.update_table(dest_mac, prev_mac, hop_count)
        # update the hop count
        hop_count+=1
        if orig_mac == self.mac:
            if dest_mac == self.enqued_msg[0]:
                next_hop = self.get_next_hop(self.enqued_msg[0])
                if next_hop:
                    # send the enqueued message
                    self.send_msg(next_hop, self.mac, self.enqued_msg[0], 0, self.enqued_msg[1])
                self.enqued_msg = None
        else:
            # get the next hop on the reverse route
            next_hop = self.get_next_hop(orig_mac)
            # TODO if no next hop, drop packet
            # send the rrep along that route
            self.send_rrep(next_hop, orig_mac, dest_mac, self.get_hop_count(dest_mac))

    def send_rreq(self, next_mac, orig_mac, dest_mac, hop_count, broadcast_id):
        self.send_data(next_mac, "F,{},{},{},{}".format(orig_mac, dest_mac, hop_count, broadcast_id))

    def send_rrep(self, next_mac, orig_mac, dest_mac, hop_count):
        self.send_data(next_mac, "FR,{},{},{}".format(orig_mac, dest_mac, hop_count))

    def send_msg(self, next_mac, orig_mac, dest_mac, hop_count, msg):
        self.send_data(next_mac, "M,{},{},{},{}".format(orig_mac, dest_mac, hop_count, msg))

    def send_error(self, next_mac, orig_mac, dest_mac):
        self.write_data(next_mac, "E,{},{}".format(orig_mac, dest_mac))

    def on_receive_msg(self, prev_mac, orig_mac, dest_mac, hop_count, msg):
        # update reverse path
        self.update_table(orig_mac, prev_mac, hop_count)

        hop_count+=1
        if dest_mac == self.mac:
            # we are the destination, handle the message
            self.handle_recv_msg(orig_mac, msg)
        else:
            next_hop = self.get_next_hop(dest_mac)
            if next_hop:
                self.send_msg(next_hop, orig_mac, dest_mac, hop_count, msg)
            else:
                next_hop = self.get_next_hop(orig_mac)
                # drop packet if we disconnected from prev
                if next_hop:
                    self.send_error(next_hop, orig_mac, dest_mac)

    # handle error from sending message from
    def on_receive_error(self, prev_mac, orig_mac, dest_mac):
        # invalidate route to destination if we route to the destination
        # from the previous mac
        self.invalidate_table(dest_mac, prev_mac)
        # try to find a route back to the orig
        next_hop = self.get_next_hop(orig_mac)
        if next_hop:
            self.send_error(next_hop, orig_mac, dest_mac)
