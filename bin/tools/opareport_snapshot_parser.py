#!/usr/bin/env python3

'''
 Loads a snapshot and parses parameters out of the fabric.
 Snapshots should be generated with the following
 ```bash
 opareport -o snapshot --route > snapshot-$(date -Iminutes).xml
 ```

Terminology
- LID (Local ID)
  - Represented as hex, but converted into integers in this script
  - Nodes will have one LID, and switches will have one per port
- GUID (Global Unique ID)
  - Represented as hex, but converted into integers in this script
- Name
  - A string identifier, similar to a hostname.
  - Switch names tend to be "{hostname}{GUID}"
  - Node names tend to be "{hostname} {Interface}" (note the space)
  - In every example I've seen, Names have a unique mapping to GUIDs
- Link
  - Snapshot enumerates connections between fabric nodes
  - Each consist of two (GUID, Port) pairs
'''


import argparse
import xml.etree.ElementTree as ET
from collections import defaultdict


def parse_args():
    parser = argparse.ArgumentParser(description='Parses reports generated by "opareport -o snapshot --route" into a JSON or YAML dictionary. Each enabled flag will populate out a key')
    parser.add_argument('-r', '--routing_table', action='store_true', help='Switch routing tables: GUID -> (lid -> port)')
    parser.add_argument('-l', '--link_table', action='store_true', help='Fabric link table: GUID -> Port -> (GUID, Port)')
    parser.add_argument('-n', '--name_guid_map', action='store_true', help='Name to GUID map: Name -> GUID')
    parser.add_argument('-g', '--guid_name_map', action='store_true', help='GUID to Name map: GUID -> Name')
    parser.add_argument('-N', '--guid_lid_map', action='store_true', help='Node to LID map: GUID -> LID')
    parser.add_argument('-L', '--lid_guid_map', action='store_true', help='LID to Node map: LID -> GUID')
    parser.add_argument('-s', '--shorten_name', action='store_true', help='Shortens string names')
    parser.add_argument('-a', '--all', action='store_true', help='The kitchen sink!')
    parser.add_argument('-o', choices={'json', 'yaml'}, default='json', help='Output format')
    parser.add_argument('FILE', help='opareport xml file name.')

    return parser.parse_args()


def parse_xml_file(fname):
    return ET.parse(fname).getroot()


def parse_hex(lid):
    return int(lid, 0)


def get_guid(xml_element):
    return parse_hex(xml_element.attrib['id'])


def xml_is_switch(xml_element):
    return xml_element.find('./NodeType_Int').text == '2'


def get_nodes(xml_root):
    for n in xml_root.findall('./Nodes/Node'):
        if not xml_is_switch(n):
            yield n


def get_switches(xml_root):
    for sw in xml_root.findall('./Nodes/Node'):
        if xml_is_switch(sw):
            yield sw


def get_links(xml_root):
    for link in xml_root.findall('./Links/Link'):
        yield link


def gen_link_table(xml_root):
    '''guid -> port -> (guid, port)'''
    link_table_ = defaultdict(dict)
    for l in get_links(xml_root):
        from_guid = parse_hex(l.find('./From/NodeGUID').text)
        from_port = int(l.find('./From/PortNum').text)
        to_guid = parse_hex(l.find('./To/NodeGUID').text)
        to_port = int(l.find('./To/PortNum').text)

        # Make the table bidirectional
        link_table_[from_guid][from_port] = (to_guid, to_port)
        link_table_[to_guid][to_port] = (from_guid, from_port)
    return dict(link_table_) # convert to dict to play more nicely with the serializer


def get_link(guid, port):
    '''link table wrapper function for simplicity'''
    return link_table[guid][port] if key in link_table else False


def gen_routing_tables(xml_root):
    '''Switch GUID -> (lid -> port) Map'''
    switch_lid_port_map = {}
    for switch in get_switches(xml_root):
        guid = get_guid(switch)
        switch_lid_port_map[guid] = {parse_hex(entry.attrib['LID']): int(entry.text)
                                     for entry in switch.findall('./SwitchData/LinearFDB/')}
    return switch_lid_port_map


def iter_node_lids(xml_root):
    '''(node GUID, LID) generator'''
    for node in get_nodes(xml_root):
        guid = get_guid(node)
        lid = node.find('./PortInfo/LID').text
        yield (guid, parse_hex(lid))


def gen_node_lid_map(xml_root):
    '''node GUID -> LID'''
    return {n: l for n, l in iter_node_lids(xml_root)}


def gen_lid_node_map(xml_root):
    '''LID -> node GUID'''
    return {l: n for n, l in iter_node_lids(xml_root)}


def iter_node_name_guids(xml_root):
    '''(name, GUID) generator'''
    for n in xml_root.findall('./Nodes/Node'):
        name = n.find('./NodeDesc').text
        guid = get_guid(n)
        yield (name, guid)


def parse_if(true, name):
    '''Parses a node name when the first argument is true'''
    return name.split()[0] if true else name


def gen_guid_name_map(xml_root, parse_name=False):
    '''GUID -> name'''
    return {g: parse_if(parse_name, n) for n, g in iter_node_name_guids(xml_root)}


def gen_name_guid_map(xml_root, parse_name=False):
    '''name -> GUID'''
    return {parse_if(parse_name, n): g for n, g in iter_node_name_guids(xml_root)}


def switch_port_for_lid(switch_id, target_lid):
    '''For a given named switch and target LID, find the port to route to'''
    return routing_table[switch_id][target_lid]


if __name__ == '__main__':
    args = parse_args()
    xml_root = parse_xml_file(args.FILE)
    out_dict = {}

    if args.routing_table or args.all:
        out_dict['routing_table'] = gen_routing_tables(xml_root)
    if args.link_table or args.all:
        out_dict['link_table'] = gen_link_table(xml_root)
    if args.name_guid_map or args.all:
        out_dict['name_guid_map'] = gen_name_guid_map(xml_root, args.shorten_name)
    if args.guid_name_map or args.all:
        out_dict['guid_name_map'] = gen_guid_name_map(xml_root, args.shorten_name)
    if args.guid_lid_map or args.all:
        out_dict['guid_map'] = gen_node_lid_map(xml_root)
    if args.lid_guid_map or args.all:
        out_dict['lid_guid_map'] = gen_lid_node_map(xml_root)

    if args.o == 'json':
        from json import dumps
        print(dumps(out_dict, indent=2))
    elif args.o == 'yaml':
        try:
            from yaml import safe_dump # safe_dump drops the annoying type tags
        except:
            print('ERROR: please install PyYAML for yaml output', file=sys.stderr)
            exit()
        print(safe_dump(out_dict, default_flow_style=False))
