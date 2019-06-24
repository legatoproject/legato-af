import json
import interfaceIR
import sys
from json import JSONEncoder


def flatten_list(l):
    """
    Recursively dig into a list of lists or values (or even a single value)
    and pull out all the values in a list.
    """

    def flatten_list_helper(l, accumulator):
        if isinstance(l, list):
            # it's a list, so descend into lists/values inside it
            for sublist in l:
                flatten_list_helper(sublist, accumulator)
        else:
            # we've reached a value (leaf node), so it should be returned
            accumulator.append(l)

    result = []
    flatten_list_helper(l, result)
    return result


def clean_comments(comments):
    """
    Flatten the unpredicable comments structure (which can be an arbitrary-depth
    list of lists), and remove blank lines from the beginning and end.
    """

    flat = flatten_list(comments)
    comments = flatten_list([line.split("\n") for line in flat])
    # often comments have blank lines at the beginning/end, so strip them:
    while len(comments) > 0 and comments[0].strip() == "":
        comments.pop(0)
    while len(comments) > 0 and comments[-1].strip() == "":
        comments.pop(-1)
    return comments


class MyEncoder(JSONEncoder):
    def default(self, o):
        o.kind = type(o).__name__
        # don't serialize location if it's None
        if hasattr(o, "location"):
            if o.location is None:
                del o.location
            elif isinstance(o.location, tuple):
                o.location = {
                    "file": o.location[0],
                    "line": o.location[1],
                    "column": o.location[2],
                }
        if hasattr(o, "iface"):
            # no need to encode interface, as the position of the node in the tree already gives us that information.
            del o.iface
        if hasattr(o, "comment"):
            # coalesce into a single attribute name for easier processing
            o.comments = o.comment
            del o.comment
        if hasattr(o, "comments"):
            o.comments = clean_comments(o.comments)
        if isinstance(o, interfaceIR.Function):
            return o.__dict__
        if isinstance(o, interfaceIR.Parameter):

            def convert(k, v):
                if k == "direction":
                    if v == interfaceIR.DIR_IN:
                        return "in"
                    elif v == interfaceIR.DIR_OUT:
                        return "out"
                    elif v == interfaceIR.DIR_INOUT:
                        return "inout"
                    else:
                        return v
                return v

            return {k: convert(k, v) for k, v in o.__dict__.iteritems()}
        if isinstance(o, interfaceIR.Type):
            return o.__dict__
        if isinstance(o, interfaceIR.Interface):
            return o.__dict__
        if isinstance(o, interfaceIR.NamedValue):
            return o.__dict__
        if isinstance(o, interfaceIR.Event):
            return o.__dict__
        if isinstance(o, interfaceIR.Definition):
            return o.__dict__
        if isinstance(o, interfaceIR.StructMember):
            return o.__dict__
        return super(MyEncoder, self).default(o)


def serialize_iface(iface):
    """
    Serialize the given IR.Interface as JSON.
    """

    # change interfaces under 'import' into objects with their (real) name and path.
    # otherwise they get serialized as nothing more than their names.
    if hasattr(iface, "imports"):

        def convert(k, v):
            if hasattr(v, "comments"):
                v.comments = clean_comments(v.comments)
            # convert imported interfaces into dictionaries, otherwise they get serialized as strings
            if isinstance(v, interfaceIR.Interface):
                return v.__dict__
            else:
                return v

        iface.imports = {k: convert(k, v) for k, v in iface.imports.iteritems()}

    if hasattr(iface, "comments"):
        iface.comments = clean_comments(iface.comments)
    return json.dumps(iface.__dict__, cls=MyEncoder, indent=2)
