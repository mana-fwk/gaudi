'''
Created on Jun 27, 2011

@author: mplajner
'''
import re
import os
import platform

class List():
    '''
    Class for manipulating with environment lists.

    It holds its name and values represented by a list.
    Some operations are done with separator, which is usually colon. For windows use semicolon.
    '''

    def __init__(self, name, local=False, report=None):
        self.report = report
        self.varName = name
        self.local = local
        self.val = []

    def name(self):
        '''Returns the name of the List.'''
        return self.varName


    def set(self, value, separator = ':', environment={}, resolve=True):
        '''Sets the value of the List. Any previous value is overwritten.'''
        if resolve:
            value = self.resolveReferences(value, environment, separator)
        else:
            value = value.split(separator)
        self.val = filter(None, value)

    def unset(self, value, separator = ':', environment={}):
        '''Sets the value of the List to empty. Any previous value is overwritten.'''
        self.val = ''


    def value(self, asString = False, separator = ':'):
        '''Returns values of the List. Either as a list or string with desired separator.'''
        if asString:
            stri = self._concatenate(separator)
            return stri.replace(']', ':')
        else:
            lis = self.val[:]
            if platform.system() != 'Linux':
                for item in lis:
                    item.replace(']',':')
            return lis


    def remove_regexp(self,value,separator = ':'):
        self.remove(value, separator, True)

    def remove(self, value, separator = ':', regexp=False):
        '''Removes value(s) from List. If value is not found, removal is canceled.'''
        if regexp:
            value = self.search(value, True)

        elif isinstance(value,str):
            value = value.split(separator)

        for i in range(len(value)):
            val = value[i]
            if val not in value:
                self.report.addWarn('Value "'+val+'" not found in List: "'+self.varName+'". Removal canceled.')
            while val in self.val:
                self.val.remove(val)


    def append(self,value, separator = ':', environment={}, warningOn=True):
        '''Adds value(s) at the end of the list.'''

        value = self.resolveReferences(value, environment, separator)

        i = 0
        while i <  len(value):
            if value[i] == '' :
                value.remove(value[i])
                continue
            if value[i] in self.val:
                if warningOn:
                    self.report.addWarn('Var: "'+self.varName+'" value: "' + value[i] + '". Addition canceled because of duplicate entry.')
                value.remove(value[i])
            else:
                i += 1

        self.val.extend(value)


    def prepend(self,value, separator = ':', environment={}):
        '''Adds value(s) at the beginning of the list.'''
        '''resolve references and duplications'''
        value = self.resolveReferences(value, environment, separator)

        i = 0
        while i <  len(value):
            if value[i] == '' :
                value.remove(value[i])
                continue
            if value[i] in self.val:
                self.report.addWarn('Var: "' + self.varName + '" value: "' + value[i] + '". Addition canceled because of duplicate entry.')
                value.remove(value[i])
            else:
                i += 1

        if isinstance(value,str):
            self.val.insert(0, value)
        else:
            i = 0
            for it in value:
                self.val.insert(0+i, it)
                i=i+1


    def search(self, expr, regExp):
        '''Searches in List`s values for a match

        Use string value or set regExp to True.
        In the first case search is done only for an exact match for one of List`s value ('^' and '$' added).
        '''
        if not regExp:
            expr = '^' + expr + '$'
        v = re.compile(expr)
        res = []
        for val in self.val:
            if v.search(val):
                res.append(val)

        return res


    def repl(self, s, d):
        v = re.compile(r"\$([A-Za-z_][A-Za-z0-9_]*)|\$\(([A-Za-z_][A-Za-z0-9_]*)\)|\$\{([A-Za-z_][A-Za-z0-9_]*)\}")
        m = v.search(s)
        if m:
            s = s[:m.start()] + str(d[filter(None,m.groups())[0]]) + s[m.end():]
            return self.repl(s,d)
        else:
            return s


    def resolveReferences(self, value, environment, separator = ':'):
        '''Resolves references to Lists'''
        if isinstance(value, list):
            str = ''
            for val in value:
                str = val + '' + separator
            value = str[:len(str)-1]

        value = self.repl(value, environment)
        value = value.split(separator)

        value = self._remDuplicates(value)
        value = self._changeSlashes(value)
        return value


    def _changeSlashes(self, value):
        '''Changes slashes depending on operating system.'''
        i = 0
        while i < len(value):
            if len(value[i]) == 0:
                del value[i]
                continue
            if value[i][0] == '[':
                if platform.system() != 'Linux':
                    value[i] = value[i][1:]
                else:
                    value[i] = value[i][3:]
            value[i] = os.path.normpath(value[i])
            i+=1
        return value


    def _concatenate(self, separator):
        '''Returns a List string with separator "separator" from the values list'''

        stri = ""
        for it in self.val:
            stri += it + separator
        stri = stri[0:len(stri)-1]
        return stri


    def _remDuplicates(self, seq, idfun=None):
        '''removes duplicated values from list'''
        if idfun is None:
            def idfun(x): return x
        seen = {}
        result = []
        for item in seq:
            marker = idfun(item)

            if marker in seen:
                self.report.addWarn('Var: "'+self.varName+'" value: "' + marker + '". Addition canceled because of duplicate entry.')
                continue
            seen[marker] = 1
            result.append(item)
        return result



    def __getitem__(self, key):
        return self.val[key]

    def __setitem__(self, key, value):
        if value in self.val:
            self.report.addWarn('Var: "' + self.varName + '" value: "' + value + '". Addition canceled because of duplicate entry.')
        else:
            self.val.insert(key, value)

    def __delitem__(self, key):
        self.removeVal(self.val[key])

    def __iter__(self):
        for i in self.val:
            yield i

    def __contains__(self, item):
        return item in self.val

    def __len__(self):
        return len(self.val)

    def __str__(self):
        return self._concatenate(':')


class Scalar():
    '''Class for manipulating with environment scalars.'''

    def __init__(self, name, local=False, report = None):
        self.report = report
        self.name = name
        self.val = ''

        self.local = local

    def name(self):
        '''Returns the name of the scalar.'''
        return self.name

    def set(self, value, separator = ':', environment={}, resolve = True):
        '''Sets the value of the scalar. Any previous value is overwritten.'''
        if resolve:
            value = self.resolveReferences(value, environment)
        self.val = value
        self._changeSlashes()
        if self.val == '.':
            self.val = ""


    def value(self, asString = False, separator = ':'):
        '''Returns values of the scalar.'''
        return self.val

    def remove_regexp(self,value,separator = ':'):
        self.remove(value, separator, True)

    def remove(self, value, separator = ':', regexp=True):
        '''Removes value(s) from the scalar. If value is not found, removal is canceled.'''
        value = self.search(value)
        for val in value:
            self.val = self.val.replace(val,'')

    def append(self,value, separator = ':', environment={}, warningOn=True):
        '''Adds value(s) at the end of the scalar.'''
        value = self.resolveReferences(value, environment)
        self.val = self.val + value
        self._changeSlashes()


    def prepend(self,value,action='cancel', separator = ':', environment={}):
        '''Adds value(s) at the beginning of the scalar.'''
        value = self.resolveReferences(value, environment)
        self.val = value + self.val
        self._changeSlashes()


    def repl(self, s, d):
        v = re.compile(r"\$([A-Za-z_][A-Za-z0-9_]*)|\$\(([A-Za-z_][A-Za-z0-9_]*)\)|\$\{([A-Za-z_][A-Za-z0-9_]*)\}")
        m = v.search(s)
        if m:
            s = s[:m.start()] + str(d[filter(None,m.groups())[0]]) + s[m.end():]
            return self.repl(s,d)
        else:
            return s


    def resolveReferences(self, value, environment):
        '''Resolve references inside the scalar.'''

        value = self.repl(value, environment)
        return value

    def search(self, expr):
        '''Searches in scalar`s values for a match'''
        return re.findall(expr,self.val)


    def _changeSlashes(self):
        '''Changes slashes depending on operating system.'''
        if self.val == '[':
            if platform.system() != 'Linux':
                self.val = self.val[1:]
            else:
                self.val = self.val[3:]
        self.val = os.path.normpath(self.val)


    def __str__(self):
        return self.val

class EnvironmentError(Exception):
    '''Class which defines errors for locals operations.'''
    def __init__(self, value, code):
        self.val = value
        self.code = code
    def __str__(self):
        if self.code == 'undefined':
            return 'Reference to undefined environment element: "'+self.val +'".'
        elif self.code == 'ref2var':
            return 'Reference to list from the middle of string.'
        elif self.code == 'redeclaration':
            return 'Wrong redeclaration of environment element "'+self.val+'".'