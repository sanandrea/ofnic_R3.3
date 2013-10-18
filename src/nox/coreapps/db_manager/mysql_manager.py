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
# @Date created 17/07/2013


from nox.lib.core     import *
import MySQLdb


class MySQLManager(Component):

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

    def install(self):
        db_conn = MySQLdb.connect(host="localhost",
                                  user="root",
                                  passwd="openflow")
        cursor = db_conn.cursor()
        sql = 'CREATE DATABASE IF NOT EXISTS netic'
        cursor.execute(sql)

    def getInterface(self):
        return str(MySQLManager)

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return MySQLManager(ctxt)

    return Factory()
