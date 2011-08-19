try:
    from collections import OrderedDict
except (ImportError, NameError):
    class OrderedDict(dict):
        def __init__(self):
            self._keys = []
        def items(self):
            return [(key, self[key]) for key in self._keys]
        def values(self):
            return [self[key] for key in self._keys]
        def keys(self):
            return self._keys[:]
        def __setitem__(self, key, value):
            if key not in self._keys:
                self._keys.append(key)
            dict.__setitem__(self, key, value)

        def __delitem__(self, key):
            if key in self._keys:
                self._keys.remove(key)
            dict.__delitem__(self, key)

__all__ = ['OrderedDict']
