import gdb.printing
import gdb.xmethod
import re


class polymorphic_value_print:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "(%s) %s" % (str(self.val["ptr_"].dynamic_type), str(self.val["ptr_"]))


def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("polymorphic_value")
    pp.add_printer(
        "polymorphic_value",
        "^isocpp_p0201::polymorphic_value<.*>$",
        polymorphic_value_print,
    )
    return pp


class polymorphic_value_method(gdb.xmethod.XMethod):
    def __init__(self, method, worker_class):
        gdb.xmethod.XMethod.__init__(self, method)
        self.worker_class = worker_class


class polymorphic_value_worker_get(gdb.xmethod.XMethodWorker):
    def get_arg_types(self):
        return None

    def get_result_type(self, obj):
        return obj["ptr_"].type

    def __call__(self, obj):
        return obj["ptr_"]


class polymorphic_value_worker_dereference(gdb.xmethod.XMethodWorker):
    def get_arg_types(self):
        return None

    def get_result_type(self, obj):
        return obj["ptr_"].dereference().type

    def __call__(self, obj):
        return obj["ptr_"].dereference()


class polymorphic_value_matcher(gdb.xmethod.XMethodMatcher):
    def __init__(self):
        gdb.xmethod.XMethodMatcher.__init__(self, "polymorphic_value")
        self._method_dict = {
            "operator->": polymorphic_value_method(
                "operator->", polymorphic_value_worker_get
            ),
            "operator*": polymorphic_value_method(
                "operator*", polymorphic_value_worker_dereference
            ),
        }
        self.methods = [self._method_dict[m] for m in self._method_dict]

    def match(self, class_type, method_name):
        if not re.match("^isocpp_p0201::polymorphic_value<.*>$", class_type.tag):
            return None
        method = self._method_dict.get(method_name)
        if method is None or not method.enabled:
            return None
        return method.worker_class()


gdb.printing.register_pretty_printer(gdb.current_objfile(), build_pretty_printer())
gdb.xmethod.register_xmethod_matcher(None, polymorphic_value_matcher())
