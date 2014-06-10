from modulegraph import modulegraph
g = modulegraph.ModuleGraph()
g.import_hook('testmod')
#g.import_hook('platform')

#n = g.findNode('platform')
#for m in g.getReferences(n):
#    print(n.identifier, "->", m.identifier, type(m).__name__)

for n in g.nodes():
    if isinstance(n, modulegraph.MissingModule):
        print(n)


print g.edges()
#
