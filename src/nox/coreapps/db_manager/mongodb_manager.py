# Copyright 2013 (C) Universita' di Roma La Sapienza
# 
# This file is part of OFNIC Uniroma GE.
# 
# OFNIC is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# OFNIC is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with OFNIC.  If not, see <http://www.gnu.org/licenses/>.

# @Author Andi Palo 
# @Date created 24/07/2013


import logging
#import pymongo

from nox.lib.core     import *
#from pymongo import MongoClient

lg = logging.getLogger('mongo')

class NoSQLManager(Component):

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

    def install(self):
        pass
        #client = MongoClient('localhost', 27017)
        #self.db = client['netic']

    def getDatabase(self):
        pass
        #return self.db
        
    def saveDocument(self, aDocument, inTable):
        lg.debug("Save doc")

    def getInterface(self):
        return str(NoSQLManager)

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return NoSQLManager(ctxt)

    return Factory()
