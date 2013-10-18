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

from nox.webapps.webservice      import webservice
from nox.lib.core import *
import simplejson as json
from nox.webapps.webservice.neticws      import *

lg = logging.getLogger('netic-api-mgr')
hdlr = logging.FileHandler('OFNIC.log')
formatter = logging.Formatter('%(asctime)s|%(name)s|%(levelname)s|%(message)s')
hdlr.setFormatter(formatter)
lg.addHandler(hdlr) 
lg.setLevel(logging.WARNING)

#
# Verifies that node is a valid extension alias is provided
#
class WSPathExistingAliasID(webservice.WSPathComponent):
    def __init__(self, apimgr):
        webservice.WSPathComponent.__init__(self)
        self._aliasws = apimgr
        
    def __str__(self):
        return "{alias}"

    def extract(self, node, data):
        if (node == None):
            return webservice.WSPathExtractResult(error="Missing end of requested URI")
        try:
            alias = node
            if  alias not in (self._apimgr.aliases): 
                e = "Alias '%s' is unknown" % node
                return webservice.WSPathExtractResult(error=e)
        except ValueError, e:        
            err = "invalid node name %s" % str(node)
            return webservice.WSPathExtractResult(error=err)
        return webservice.WSPathExtractResult(node)

class NeticApiMgr(Component):
















