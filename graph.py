import networkx as nx
import matplotlib as mpl
mpl.use('TkAgg')
import matplotlib.pyplot as plt
import matplotlib.animation
import time
import threading
from routing_api import GraphEntry
import os

class Graph(threading.Thread):
  def __init__(self):
    threading.Thread.__init__(self)
    self.graph = {}
    self.updated = True
  
  def set_graph(self, graph):
    if (graph != self.graph):
      self.updated = True
    self.graph = graph.copy()

  def run(self):
    # Build plot
    fig, ax = plt.subplots(figsize=(6,4))

    def update(num):
      if self.updated:
        self.updated = False
        ax.clear()
        print(num)
        G = nx.DiGraph()
        edges = []
        for sta_mac in self.graph:
          for ap_mac in self.graph[sta_mac].adj_node_dict:
            if self.graph[sta_mac].adj_node_dict[ap_mac] == 1:
              f = str(int(sta_mac) & 0xff)
              t = str(int(ap_mac) & 0xff)
              edges.append((f, t))

        G.add_edges_from(edges)

        # Need to create a layout when doing
        # separate calls to draw nodes and edges
        pos = nx.spring_layout(G)
        nx.draw_networkx_nodes(G, pos, cmap=plt.get_cmap('jet'), 
                              node_size = 500)
        nx.draw_networkx_labels(G, pos)
        nx.draw_networkx_edges(G, pos, edgelist=edges, edge_color='r', arrows=True)

    ani = matplotlib.animation.FuncAnimation(fig, update, frames=2, interval=1000, repeat=True)
    plt.show()