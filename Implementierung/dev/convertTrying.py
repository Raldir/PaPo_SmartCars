"""
Parser that extract the edge and vertex information of an OSM document in two seperate files
Partly use of the code of Brandon Martin-Anderson under the BSD License http://github.com/brianw/osmgeocode
"""
import xml.sax
import copy

def mapVertices(vertices):
    iDMapping = {}
    count = 0
    for node in vertices:
        iDMapping[node.id] = count
        count+=1
    
    return iDMapping

def twopointWays(i, size, file, nodes, edge):
    if size > 2:
        nodes = nodes + [edge.nds[i]] + [edge.nds[i+1]]
        file.write(edge.nds[i] + " " + edge.nds[i + 1] + '\n')
        return nodes + twopointWays(i + 1, size - 1, file, nodes, edge)
    else:
        nodes = nodes + [edge.nds[i]] + [edge.nds[i+1]]
        file.write(edge.nds[i] + " " + edge.nds[i + 1])
        file.write('\n')
        #print(nodes)
        return nodes

def read_osm(filename_or_stream, only_roads=True):
    "TODO Nomalisiere Koordinaten der Nodes"
    "TODO Speichere Relevanz der Straße"
    osm = OSM(filename_or_stream)
    nodes = []
    for key, edge in osm.ways.items():
        if 'highway' not in edge.tags:
            continue
            #nodes  = nodes + twopointWays(0, len(edge.nds), f, [], edge)
            #print(i)
            #Füge alle Straßentypen ein die beachtet werden, können hier abgelesen werden: http://wiki.openstreetmap.org/wiki/Key:highway
            #edge.tags.get('highway') != 'crossing'\ and edge.tags.get('highway') != 'roundabout' and edge.tags.get('highway') != 'unclassified' 
        if edge.tags.get('highway') != 'residential' and edge.tags.get('highway') != 'turning_circle' and edge.tags.get('highway') != 'crossing' and edge.tags.get('highway') != 'motorway'\
        and edge.tags.get('highway') != 'primary' and edge.tags.get('highway') != 'secondary' and edge.tags.get('highway') != 'tertiary'\
        and edge.tags.get('highway') != 'trunk' and edge.tags.get('highway') != 'trunk_link' and edge.tags.get('highway') != 'motorway_link':
            continue
        for i in range(0, len(edge.nds)):
            if i == 0 or i == len(edge.nds) - 1:
                nodes = nodes + [edge.nds[i]]
    nodesFiltered = []
    for key, node in osm.nodes.items():
        #print(node.id)
        if node.id in nodes:
            nodesFiltered = nodesFiltered + [node]
    mappedID = mapVertices(nodesFiltered)


    f = open('dev/OutputMap/edges', 'w')
    for key, edge in osm.ways.items():
        if 'highway' not in edge.tags:
            continue
            #nodes  = nodes + twopointWays(0, len(edge.nds), f, [], edge)
            #print(i)
        if edge.tags.get('highway') != 'residential'and edge.tags.get('highway') != 'turning_circle' and edge.tags.get('highway') != 'crossing' and edge.tags.get('highway') != 'motorway'\
        and edge.tags.get('highway') != 'primary' and edge.tags.get('highway') != 'secondary' and edge.tags.get('highway') != 'tertiary'\
        and edge.tags.get('highway') != 'trunk' and edge.tags.get('highway') != 'trunk_link' and edge.tags.get('highway') != 'motorway_link':
            continue
        for i in range(0, len(edge.nds)):
            tup1 = ()
            if i == 0 or i == len(edge.nds) - 1:
                f.write(str(mappedID[edge.nds[i]]) + " ")
        f.write('\n')
    f.close()
    #values = positions(osm.nodes, nodes)
    f = open('dev/OutputMap/edges', 'r')
    counts = {}
    connection = {}
    connectionback= {}
    for line in f :
        arguments = line.split()
        counts[arguments[0]] = 0
        counts[arguments[1]] = 0
        connection[arguments[0]] = []
        connectionback[arguments[1]] = []
    f.close()
    f = open('dev/OutputMap/edges', 'r')
    for line in f :
        arguments = line.split()
        counts[arguments[0]] +=1
        counts[arguments[1]] +=1
        connection[arguments[0]].append(arguments[1])
        connectionback[arguments[1]].append(arguments[0])
    f.close()
    spawner = []
    for key in counts:
        if counts[key] != 2:
            spawner.append(key)
    for key in spawner:
        if(key == "177"):
            print(key)
        if key in connection:
            for nextv in connection[key]:
                next = nextv
                while counts[next] == 2 and dicHas(next, connection):
                    nextv = next
                    next = connection[next][0]
    for key in spawner:
       if key in connectionback:
           for nextv in connectionback[key]:
                next = nextv
                while counts[next] == 2 and dicHas(next, connectionback):
                    nextv = next
                    next = connectionback[next][0]

    f = open('dev/OutputMap/edges', 'w')
    for key in counts:
        if key in connection and counts[key] != 2:
            #print(key)
            for value in connection[key]:
                f.write(key + " " + value + '\n')
        if key in connectionback and counts[key] != 2:
            #print(key)
             for value in connectionback[key]:
                f.write(key + " " + value + '\n')
    f.close()
    #for key in connection :
        #print(type(key))
        #print(key+ " ")
    f = open('dev/OutputMap/nodes', 'w')
    for key, node in osm.nodes.items():
        #print(node.id)
        #and counts[str(mappedID[node.id])] != 2
        if node.id in nodes and counts[str(mappedID[node.id])] != 2:
            #print(type(node.id))
            f.write(str(mappedID[node.id]) + " " + str(node.x * 1000) + " " + str(node.y * 1000) + '\n')
    f.close()

def dicHas(value, dic):
    for key in dic:
        for valuet in key:
            if(valuet == value):
                return True
    return False
"""
def positions(allnodes, nodes):
    meanX = 0
    meanY = 0
    varX = 0
    varY = 0
    for key, node in allnodes.items():
        if(node.id in nodes):
            meanX += node.x
            meanY += node.y
    meanX /= len(nodes)
    meanY /= len(nodes)
    for key, node in allnodes.items():
        if(node.id in nodes):
            varX += (node.x - meanX)**2
            varY += (node.y - meanY)**2
    varX /= len(nodes)
    varY /= len(nodes)
    return [meanX, meanY, varX, varY]
"""

class Node:
    def __init__(self, id, x, y):
        self.id = id
        self.x = x
        self.y = y
        self.tags = {}
        
class Way:
    def __init__(self, id, osm):
        self.osm = osm
        self.id = id
        self.nds = []
        self.tags = {}
        
    def split(self, dividers):
        # slice the node-array using this nifty recursive function
        def slice_array(ar, dividers):
            for i in range(1,len(ar)-1):
                if dividers[ar[i]]>1:
                    #print "slice at %s"%ar[i]
                    left = ar[:i+1]
                    right = ar[i:]
                    
                    rightsliced = slice_array(right, dividers)
                    
                    return [left]+rightsliced
            return [ar]
            
        slices = slice_array(self.nds, dividers)
        
        # create a way object for each node-array slice
        ret = []
        i=0
        for slice in slices:
            littleway = copy.copy( self )
            littleway.id += "-%d"%i
            littleway.nds = slice
            ret.append( littleway )
            i += 1
            
        return ret
        

class OSM:
    "TODO Stelle auf Tree um, ist effizienter"
    def __init__(self, file):
        nodes = {}
        ways = {}
        superself = self
        
        class OSMHandler(xml.sax.ContentHandler):
            @classmethod
            def setDocumentLocator(self,loc):
                pass
            
            @classmethod
            def startDocument(self):
                pass
                
            @classmethod
            def endDocument(self):
                pass
                
            @classmethod
            def startElement(self, name, attrs):
                if name=='node':
                    self.currElem = Node(attrs['id'], float(attrs['lon']), float(attrs['lat']))
                elif name=='way':
                    self.currElem = Way(attrs['id'], superself)
                elif name=='tag':
                    self.currElem.tags[attrs['k']] = attrs['v']
                elif name=='nd':
                    self.currElem.nds.append( attrs['ref'] )
                
            @classmethod
            def endElement(self,name):
                if name=='node':
                    nodes[self.currElem.id] = self.currElem
                elif name=='way':
                    ways[self.currElem.id] = self.currElem
                
            @classmethod
            def characters(self, chars):
                pass

        xml.sax.parse(file, OSMHandler)
        
        self.nodes = nodes
        self.ways = ways
            
        #count times each node is used
        node_hash = dict.fromkeys(self.nodes.keys(), 0 )
        for way in self.ways.values():
            if len(way.nds) < 2:       #if a way has only one node, delete it out of the osm collection
                del self.ways[way.id]
            else:
                for node in way.nds:
                    node_hash[node] += 1
        new_ways = {}
        for id, way in self.ways.items():
            split_ways = way.split(node_hash)
            for split_way in split_ways:
                new_ways[split_way.id] = split_way
        self.ways = new_ways
     
read_osm('dev/InputMap/map.osm')        