# protobuf_parser.py
#
#  simple parser for parsing protobuf .proto files
#
#  Copyright 2010, Paul McGuire
#

"""
Edited by: dmakarov
Time: 26 april 2018 12:02
Comments: parse services and RPCs
"""

from pyparsing import (Word, alphas, alphanums, Regex, Suppress, Forward,
    Group, oneOf, ZeroOrMore, Optional, delimitedList, Keyword,
    restOfLine, quotedString, Dict)

ident = Word(alphas+"_'\"",alphanums+"_'\"").setName("identifier")
integer = Regex(r"[+-]?\d+")

LBRACE,RBRACE,LBRACK,RBRACK,LPAR,RPAR,EQ,SEMI = map(Suppress,"{}[]()=;")

kwds = """message required optional repeated enum extensions extends extend 
          to package service rpc returns true false option import syntax stream"""

for kw in kwds.split():
    exec("%s_ = Keyword('%s')" % (kw.upper(), kw))

messageBody = Forward()

messageDefn = MESSAGE_ - ident("messageName") + LBRACE + messageBody("body") + RBRACE

typespec = oneOf("""double float int32 int64 uint32 uint64 sint32 sint64 
                    fixed32 fixed64 sfixed32 sfixed64 bool string bytes""") | ident
""""""
rvalue = integer | TRUE_ | FALSE_ | ident
fieldDirective = LBRACK + Group(ident + EQ + rvalue) + RBRACK
fieldDefn = (Optional(( REQUIRED_ | OPTIONAL_ | REPEATED_ )("fieldQualifier")) + 
              typespec("typespec") + ident("ident") + EQ + integer("fieldint") + ZeroOrMore(fieldDirective) + SEMI)

# enumDefn ::= 'enum' ident '{' { ident '=' integer ';' }* '}'
enumDefn = ENUM_("typespec") - ident('name') + LBRACE + Dict( ZeroOrMore( Group(ident + EQ + integer + SEMI) ))('values') + RBRACE

# extensionsDefn ::= 'extensions' integer 'to' integer ';'
extensionsDefn = EXTENSIONS_ - integer + TO_ + integer + SEMI

# messageExtension ::= 'extend' ident '{' messageBody '}'
messageExtension = EXTEND_ - ident + LBRACE + messageBody + RBRACE

# messageBody ::= { fieldDefn | enumDefn | messageDefn | extensionsDefn | messageExtension }*
messageBody << Group(ZeroOrMore( Group(fieldDefn | enumDefn | messageDefn | extensionsDefn | messageExtension) ))

# methodDefn ::= 'rpc' ident '(' [ ident ] ')' 'returns' '(' [ ident ] ')' ';'
methodDefn = (RPC_ - ident("methodName") + 
              LPAR + Optional(STREAM_("paramStreamQualifier")) + Optional(ident("methodParam")) + RPAR + 
              RETURNS_ + LPAR + Optional(STREAM_("returnStreamQualifier")) + Optional(ident("methodReturn")) + RPAR + ((LBRACE + RBRACE) | SEMI))

# serviceDefn ::= 'service' ident '{' methodDefn* '}'
serviceDefn = SERVICE_ - ident("serviceName") + LBRACE + ZeroOrMore( Group(methodDefn) )('RPCs') + RBRACE

# packageDirective ::= 'package' ident [ '.' ident]* ';'
packageDirective = Group(PACKAGE_ - delimitedList(ident('packageName'), '.', combine=True) + SEMI)

syntaxDirective = Group(SYNTAX_ - EQ + ident + SEMI)

comment = '//' + restOfLine

importDirective = IMPORT_ - quotedString("importFileSpec") + SEMI

optionDirective = OPTION_ - ident("optionName") + EQ + quotedString("optionValue") + SEMI

topLevelStatement = Group(messageExtension | enumDefn | importDirective | optionDirective| serviceDefn | messageDefn )# | Group(serviceDefn)('services') | Group(messageDefn)('messages')

parser = Optional(syntaxDirective)("syntax") + (Optional(packageDirective) + ZeroOrMore(topLevelStatement))('package')

parser.ignore(comment)


test1 = """message Person { 
  required int32 id = 1; 
  required string name = 2; 
  optional string email = 3; 
}"""

test2 = """package tutorial;

message Person {
  required string name = 1;
  required int32 id = 2;
  optional string email = 3;

  enum PhoneType {
    MOBILE = 0;
    HOME = 1;
    WORK = 2;
  }

  message PhoneNumber {
    required string number = 1;
    optional PhoneType type = 2 [default = HOME];
  }

  repeated PhoneNumber phone = 4;
}

message AddressBook {
  repeated Person person = 1;
}"""

test3 = """\
syntax = "proto3";

//option java_multiple_files = true;
//option java_package = "io.grpc.examples.helloworld";
//option java_outer_classname = "HelloWorldProto";
//option objc_class_prefix = "HLW";

package pingpong;

// The greeting service definition.
service ping
{
  // Sends a greeting
  rpc SayHello (PingRequest) returns (PingReply) {}

  rpc GladToSeeMe(PingRequest) returns (stream PingReply){}

  rpc GladToSeeYou(stream PingRequest) returns (PingReply){}

  rpc BothGladToSee(stream PingRequest) returns (stream PingReply){}
}

// The request message containing the user's name.
message PingRequest
{
  string name = 1;
  string message = 2;
}

// The response message containing the greetings
message PingReply
{
  string message = 1;
}
"""

def runAllTests():
    parser.runTests([test1, test2, test3])
    r = parser.parseString(test3)
    print(len(r.asDict()['package']))
    print(r.asDict()['package'][0])
    print(r.asDict()['package'][1])
    print(r.asDict()['package'][2])
    print(r.asDict()['package'])
    for k in r.items():
        print(k)


class grpc_rpc:
    def __init__(self):
        self.rpcname = ''
        self.paramtype = ''
        self.paramisstream = False
        self.returntype = ''
        self.returnisstream = False    
    def __str__(self):
        return 'RPC{\nname: %s\nparam: %s\nreturn: %s\npstream: %s\nrstream: %s\n}' % (self.rpcname, self.paramtype, self.returntype, self.paramisstream, self.returnisstream)

class grpc_service:
    def __init__(self, name):
        self.rpc_list = []
        self.name = name


def parseProtoFile(path):
    try:
        with open(path) as f:
            s = ''.join(f.readlines())
        #parser.runTests([s])
        res = parser.parseString(s)
    except Exception as e:
        print(e)
        return None
    
    d = res.asDict()
    #print(d)
    if not d: return None
    try:
        package = d['package']
    except KeyError: return []
    services = []
    messages = []
    packageName = ''
    for smth in package:
        if 'messageName' in smth.keys():
            messages.append(smth['messageName'])
            continue
        if 'packageName' in smth.keys():
            assert(not packageName)
            packageName = smth['packageName']
            continue
        if not 'serviceName' in smth.keys(): continue
        service = grpc_service(smth['serviceName'])
        for r in smth['RPCs']:
            rpc = grpc_rpc()
            rpc.rpcname = r['methodName']
            rpc.paramtype = r.get('methodParam', '')
            rpc.returntype = r.get('methodReturn', '')
            rpc.paramisstream = 'paramStreamQualifier' in r.keys()
            rpc.returnisstream = 'returnStreamQualifier' in r.keys()
            service.rpc_list.append(rpc)
        #yield service
        services.append(service)
    assert(packageName)
    return packageName, services, messages
    

def printServices(services):
    for s in services:
        print(s.name+' : ')
        for rpc in s.rpc_list:
            print(rpc)
        print()

#pckg, services, messages = parseProtoFile('/home/user/projects/meteo-branch/src/meteo/proto/pingpong.proto')
#print(pckg)
#printServices(services) 
#print(messages)
