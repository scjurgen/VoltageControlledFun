#!/usr/bin/env python3

'''
small script to create graph from the audio modules.
far from complete and nice, but useful
'''

import re
import sys

from graphviz import Digraph

mapShapes = {
    'AudioSynthWaveformDc': 'circle',
    'AudioEffectFade': 'egg',
    'AudioNoiseGate': 'invtriangle',
    'AudioMixer4': 'component',
    'AudioInputI2S': 'box',
    'AudioOutputI2S': 'box',
    'AudioEffectDelay': 'tripleoctagon',
    'AudioFilterStateVariable':'diamond',
    'AudioFilterStateVariableLimited':'diamond',
    'AudioFilterBiquad': 'diamond',
    'AudioAnalyzeRMS': 'triangle',
    'AudioAnalyzePeak': 'triangle',
    'AudioFilterResonance': 'diamond',
    'AudioControlSGTL5000': 'box',
    'AudioRecordAndHoldSampler': 'tripleoctagon'
}


def analyse(fname):
    g = Digraph('G', filename='audiochain.gv')
    nodeNames = dict()
    nodes = set()
    edges = list()
    with open(fname) as f:
        for line in f:
            m = re.match("^(Audio[a-zA-Z0-9]+)\\s+([^ ]+)\\s*;.*$", line)
            if m:
                nodes.add(m.group(2))
                print(m.group(2))
                mg = re.match("(.*)\\[([0-9]+)\\]", m.group(2))
                if mg:
                    for i in range(int(mg.group(2))):
                        nodeNames["{}[{}]".format(mg.group(1), i)] = m.group(1)
                else:
                    nodeNames[m.group(2)] = m.group(1)
            m = re.match("^AudioConnection\\s+([a-zA-Z0-9]+)\\s*\\(([^, ]*),\\s*([^, ]*),\\s*([^, ]*),\\s*([^, ]*)\\)(.*)$", line)
            if m:
                nodes.add(m.group(2))
                nodes.add(m.group(4))
                edges.append([m.group(2),m.group(4),m.group(3),m.group(5)])
    for node in nodes:
        g.node(node, shape=mapShapes[nodeNames[node]])

    for fromEdge, toEdge, taillable,headlable in edges:
        g.edge(fromEdge, toEdge, arrowhead='odot', headlable=headlable, taillabel=taillable)

    g.graph_attr['rankdir'] = 'LR'
    g.view()


def run_main():
    if len(sys.argv) < 2:
        print("give me the filename to scan for audio modules") 
        sys.exit(2)
    analyse(sys.argv[1])

if __name__ == "__main__":
    run_main()



