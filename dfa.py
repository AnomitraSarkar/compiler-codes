import matplotlib.pyplot as plt
import networkx as nx

# Redefine for stylistic DFA like in the uploaded image
dfa_nodes = {
    0: "{1,2,3}",
    1: "{1,2,3,4,5,6,7}",
    2: "{2,3}",
    3: "{1,2,3,4,5,6,7,8}",  # Accept state
    4: "{2,3,4,5,6,7}",
    5: "{4,5,6,7}",
    6: "{4,5,6,7,8}"
}

dfa_edges = [
    (0, 1, 'a'),
    (0, 2, 'b'),
    (1, 3, 'a'),
    (1, 4, 'b'),
    (2, 5, 'a'),
    (2, 2, 'b'),
    (4, 5, 'a'),
    (4, 6, 'b'),
    (5, 6, 'a'),
    (5, 5, 'b')
]

accept_state = 3

# Build styled graph
G = nx.DiGraph()
G.add_edges_from([(u, v, {'label': lbl}) for u, v, lbl in dfa_edges])

pos = nx.spring_layout(G, seed=5)

# Plot graph
plt.figure(figsize=(14, 8))
node_labels = {i: dfa_nodes[i] for i in dfa_nodes}
node_colors = ['lightgreen' if i == accept_state else 'lightblue' for i in G.nodes]
node_border_widths = [3 if i == accept_state else 1 for i in G.nodes]

nx.draw(G, pos, labels=node_labels, node_size=3000, node_color=node_colors,
        linewidths=node_border_widths, edgecolors='black', font_size=12)
edge_labels = nx.get_edge_attributes(G, 'label')
nx.draw_networkx_edge_labels(G, pos, edge_labels=edge_labels, font_color='red', font_size=12)
plt.title("DFA for a*b*a(a|b)*b*a#", fontsize=16)
plt.axis('off')
plt.show()
