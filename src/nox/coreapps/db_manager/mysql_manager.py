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

# @Updated by Valerio Di Matteo, 09/10/2013


from nox.lib.core     import *
import MySQLdb

MYSQL_HOST = "localhost"
MYSQL_PORT = 3306
MYSQL_USER = "root"
MYSQL_PASSWD = "openflow"
MYSQL_DB = "openflow_users" 

#Conn = MySQLdb.Connect(host = MYSQL_HOST, port = MYSQL_PORT, user = MYSQL_USER, passwd= MYSQL_PASSWD, db= MYSQL_DB )
#Cursor = Conn.cursor(MySQLdb.cursors.DictCursor) #Permette l'accesso attraverso il nome dei fields


def check_db_connection(fn):
    def wrapped(*args):
        retries = 5;
        #Conn = None
        while (retries > 0):
            retries -= 1;
            try:
                Conn = MySQLdb.Connect(host = MYSQL_HOST, port = MYSQL_PORT, user = MYSQL_USER, passwd= MYSQL_PASSWD, db= MYSQL_DB )
                args[0].cursor = Conn.cursor(MySQLdb.cursors.DictCursor)
                return fn(*args)
            except MySQLdb.Error, e:
                pass
                #if Conn:
                    #Conn.close
                #Conn = MySQLdb.Connect(host = MYSQL_HOST, port = MYSQL_PORT, 
                                       #user = MYSQL_USER, passwd = MYSQL_PASSWD, 
                                       #db = MYSQL_DB)
        #Conn.close()
        return None
    return wrapped


class MySQLManager(Component):
    db = None
    cursor = None
    
    
    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

    def install(self):
        print "Installing module mysql_manager"
        
    def echo(self, msg):
        print msg
    
    @check_db_connection
    def call_roles_db(self):
        query = "SELECT DISTINCT Role FROM editable_roles;"
        self.cursor.execute(query)
        data=self.cursor.fetchall()
        return data
    
    @check_db_connection
    def check_role(self, role):
        query = "SELECT Role FROM editable_roles where Role='"+role+"';"
        self.cursor.execute(query)
        if self.cursor.rowcount>0:
            return True
        else:
            return False

    @check_db_connection
    def check_role_all(self, role):
        query = "SELECT Name FROM roles where Name='"+role+"';"
        self.cursor.execute(query)
        if self.cursor.rowcount>0:
            return True
        else:
            return False


    @check_db_connection
    def remove_role(self,role):
        query = "delete FROM editable_roles where Role='"+role+"';"
        self.cursor.execute(query)   
        query = "delete from user_roles where Role='"+role+"';"
        self.cursor.execute(query)

    @check_db_connection        
    def call_users_db(self):
        query = "SELECT username FROM users;"
        self.cursor.execute(query)
        data=self.cursor.fetchall()
        return data

    def call_all_user_roles_db(self):
        query = "SELECT User,GROUP_CONCAT(Role SEPARATOR ',') as Roles  FROM user_roles GROUP BY User;"
        self.cursor.execute(query)
        data=self.cursor.fetchall()
        return data

    @check_db_connection        
    def get_all_roles_db(self):
        query = "SELECT Name FROM roles;"
        self.cursor.execute(query)
        data=self.cursor.fetchall()
        return data

    @check_db_connection        
    def check_user(self, username):
        query = "SELECT username FROM users where username='"+username+"';"
        self.cursor.execute(query)
        if self.cursor.rowcount>0:
            return True
        else:
            return False


    @check_db_connection            
    def reg_user(self,username,password):
        query="insert into users (username,password,language) values ('"+username+"','"+password+"','en');"
        self.cursor.execute(query)
        self.add_role_to_user(username,"Readonly");


    @check_db_connection     
    def remove_user(self,username):
        query = "delete FROM users where username='"+username+"';"
        self.cursor.execute(query)   
        query = "delete from user_roles where User='"+username+"';"
        self.cursor.execute(query)
 
    @check_db_connection
    def call_cap_db(self):
        query = "SELECT * FROM capabilities;"
        self.cursor.execute(query)
        data=self.cursor.fetchall()
        return data
        
    @check_db_connection
    def check_cap(self, cap):
        query = "SELECT Name FROM capabilities where Name='"+cap+"';"
        self.cursor.execute(query)
        if self.cursor.rowcount>0:
            return True
        else:
            return False
    
    @check_db_connection
    def get_cap_by_role(self, name):
        query = "SELECT Cap FROM editable_roles where Role='"+name+"';"
        self.cursor.execute(query)
        data=self.cursor.fetchall()
        return data

    @check_db_connection
    def get_other_cap_by_role(self, name):
        query = "SELECT Name FROM capabilities where Name not in (Select Cap FROM editable_roles where Role='"+name+"');"
        self.cursor.execute(query)
        data=self.cursor.fetchall()
        return data

    @check_db_connection
    def authenticate(self, username, password):
        query = "SELECT * FROM users where username='"+username+"' and password='"+password+"';"
        self.cursor.execute(query)
        if self.cursor.rowcount > 0:
            query = "SELECT * FROM user_roles where User='"+username+"';"
            self.cursor.execute(query)
            data=self.cursor.fetchall()
            return data    
        else:
            return False
    
    @check_db_connection
    def add_new_role(self,role):
        query = "select * from editable_roles where Role='"+role+"';"
        self.cursor.execute(query)
        if self.cursor.rowcount > 0:
            return False   
        else:
            query = "insert into editable_roles (Role,Cap) values('"+role+"','GET');"
            self.cursor.execute(query)
            return True
    
    @check_db_connection
    def add_cap_to_role(self,role,cap):
        query = "select * from editable_roles where Role='"+role+"' and Cap='"+cap+"';"
        self.cursor.execute(query)
        if self.cursor.rowcount > 0:
            return False   
        else:
            query = "insert into editable_roles (Role,Cap) values('"+role+"','"+cap+"');"
            self.cursor.execute(query)
            return True

    @check_db_connection
    def del_cap_from_role(self,role,cap):
        query = "delete from editable_roles where Role='"+role+"' and Cap='"+cap+"';"
        self.cursor.execute(query)
        if self.cursor.rowcount > 0:
            return True   
        else:
            return False

    @check_db_connection    
    def db_path(self,path,method):
        if (path.count("/statistics/task/")>0):
            if(method=="DELETE"):
                path="DELETE_STAT"
            elif(method=="GET"):
                path= "GET_STAT"
        elif (path.count("/statistics/node/")>0):
            path="GET_NODE"
        elif (path.count("/virtualpath/")>0):
            if(method=="GET"):
                path="GET_PATH"
            elif(method=="DELETE"):
                path="DELETE_PATH"
        elif (path.count("/synchronize/network/node/")>0):
            path="SYNC_NODE"
        elif (path.count("/extension/")>0):
            path="EXTENSION"
        elif (path.count("/controlpanel/")>0):
            path="CONTROL_PANEL"
        return path

    @check_db_connection
    def check_path(self,request):
        path=self.db_path(request.uri,request.method)
        query = "SELECT Cap FROM resources where Path='"+path+"';"    
        self.cursor.execute(query)
        if self.cursor.rowcount>0:
            data=self.cursor.fetchall()
            return data
        else:
            return False
    
    @check_db_connection
    def get_res(self):
        #query = "SELECT distinct Path FROM resources;"
        query = "SELECT Path,GROUP_CONCAT(Cap SEPARATOR ',') as Caps  FROM resources GROUP BY Path;"
        self.cursor.execute(query)
        data=self.cursor.fetchall()
        return data
    
    @check_db_connection    
    def check_res(self, path):
        query = "SELECT Path FROM resources where Path='"+path+"';"
        self.cursor.execute(query)
        if self.cursor.rowcount>0:
            return True
        else:
            return False
    
    @check_db_connection
    def get_cap_by_res(self,res):
        query = "SELECT Cap FROM resources where Path='"+res+"';"
        self.cursor.execute(query)
        data=self.cursor.fetchall()
        return data
    
    @check_db_connection
    def res_has_caps(self,res):
        query = "SELECT Cap FROM resources where Path='"+res+"';"
        self.cursor.execute(query)
        if self.cursor.rowcount>0:
            return True
        else:
            return False
    
    @check_db_connection
    def get_other_cap_by_res(self,res):
        query = "SELECT Name FROM capabilities where Name not in (Select Cap FROM resources where Path='"+res+"');"
        self.cursor.execute(query)
        data=self.cursor.fetchall()
        return data
    
    @check_db_connection
    def add_cap_to_res(self,res,cap):
        query = "select * from resources where Path='"+res+"' and Cap='"+cap+"';"
        self.cursor.execute(query)
        if self.cursor.rowcount > 0:
            return False   
        else:
            query = "insert into resources (Path,Cap) values('"+res+"','"+cap+"');"
            self.cursor.execute(query)
            return True

    @check_db_connection    
    def del_cap_from_res(self,res,cap):
        query = "delete from resources where Path='"+res+"' and Cap='"+cap+"';"
        self.cursor.execute(query)
        if self.cursor.rowcount > 0:
            return True   
        else:
            return False
    
    @check_db_connection
    def get_role_by_user(self,user):
        query = "SELECT Role FROM user_roles where User='"+user+"';"
        self.cursor.execute(query)
        data=self.cursor.fetchall()
        return data
    

    @check_db_connection
    def get_other_role_by_user(self,user):
        query = "SELECT Name FROM roles where Name not in (SELECT Role FROM user_roles where User='"+user+"');"
        self.cursor.execute(query)
        data=self.cursor.fetchall()
        return data

    @check_db_connection
    def add_role_to_user(self,user,role):
        query = "select * from user_roles where User='"+user+"' and Role='"+role+"';"
        self.cursor.execute(query)
        if self.cursor.rowcount > 0:
            return False   
        else:
            query = "insert into user_roles (User,Role) values('"+user+"','"+role+"');"
            self.cursor.execute(query)
            return True
    
    @check_db_connection    
    def del_role_from_user(self,user,role):
        query = "delete from user_roles where User='"+user+"' and Role='"+role+"';"
        self.cursor.execute(query)
        if self.cursor.rowcount > 0:
            return True   
        else:
            return False
    
    def getInterface(self):
        return str(MySQLManager)

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return MySQLManager(ctxt)

    return Factory()
