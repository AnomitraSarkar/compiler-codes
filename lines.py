import matplotlib.pyplot as plt

# Create a custom syntax tree diagram like the one in the image

fig, ax = plt.subplots(figsize=(14, 10))
ax.axis('off')

# Draw nodes with annotations manually for the regex a*b*a(a|b)*b*a#
nodes = [
    # (x, y, label, color)
    (0.5, 1.0, '.', 'black'),

    (0.3, 0.85, '.', 'black'),
    (0.1, 0.7, '*', 'red'),

    (0.05, 0.55, 'a', 'blue'),
    (0.15, 0.55, 'b', 'blue'),

    (0.3, 0.7, 'a', 'blue'),
    (0.45, 0.7, '.', 'black'),
    (0.4, 0.55, '(', 'white'),  # dummy
    (0.5, 0.55, '|', 'black'),

    (0.45, 0.4, 'a', 'blue'),
    (0.55, 0.4, 'b', 'blue'),

    (0.65, 0.85, '.', 'black'),
    (0.6, 0.7, '*', 'red'),
    (0.55, 0.55, 'b', 'blue'),

    (0.7, 0.7, 'a', 'blue'),
    (0.8, 0.7, '#', 'blue')
]

# Plot the nodes
for (x, y, label, color) in nodes:
    ax.text(x, y, label, ha='center', va='center',
            fontsize=14, bbox=dict(boxstyle="circle", fc='white', ec=color if color != 'white' else 'white', lw=2))

# Draw arrows manually to replicate syntax tree edges
edges = [
    ((0.5, 1.0), (0.3, 0.85)),
    ((0.5, 1.0), (0.65, 0.85)),
    ((0.3, 0.85), (0.1, 0.7)),
    ((0.3, 0.85), (0.3, 0.7)),
    ((0.1, 0.7), (0.05, 0.55)),
    ((0.1, 0.7), (0.15, 0.55)),
    ((0.65, 0.85), (0.6, 0.7)),
    ((0.65, 0.85), (0.7, 0.7)),
    ((0.6, 0.7), (0.55, 0.55)),
    ((0.55, 0.55), (0.45, 0.4)),
    ((0.55, 0.55), (0.55, 0.4)),
]

for (x1, y1), (x2, y2) in edges:
    ax.annotate("",
                xy=(x2, y2), xycoords='data',
                xytext=(x1, y1), textcoords='data',
                arrowprops=dict(arrowstyle="->", lw=1.5))

# Add text annotations for firstpos, lastpos, followpos
info_text = [
    "Pos   Sym   FirstPos   LastPos   FollowPos",
    " 1     a     {1}         {1}       {1,2,3}",
    " 2     b     {2}         {2}       {1,2,3}",
    " 3     a     {3}         {3}       {4,5,6,7}",
    " 4     a     {4}         {4}       {5,6,7}",
    " 5     b     {5}         {5}       {6,7}",
    " 6     b     {6}         {6}       {7}",
    " 7     a     {7}         {7}       {8}",
    " 8     #     {8}         {8}         {}",
]

for i, line in enumerate(info_text):
    ax.text(1.05, 0.95 - i * 0.05, line, fontsize=12, family='monospace')

plt.title("Syntax Tree and Followpos Table for a*b*a(a|b)*b*a#", fontsize=15)
plt.tight_layout()
plt.show()
