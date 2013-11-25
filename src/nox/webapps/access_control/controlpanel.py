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

# @Author Valerio Di Matteo 
# @Date created 09/10/2013


from nox.lib.core     import *
from nox.webapps.webserver import webauth
from nox.webapps.webservice      import webservice
from nox.coreapps.db_manager import mysql_manager
from nox.netapps.discovery.discoveryws import *
from nox.webapps.webservice.neticws      import *

import simplejson as simplejson

class ControlTools(Component):
      
    def __init__(self, ctxt):
        
        Component.__init__(self, ctxt)
        

    def install(self):
       
        self.manager=self.resolve(str(mysql_manager.MySQLManager))
        
        self.discoveryws = self.resolve(discoveryws)
        
        ws  = self.resolve(str(webservice.webservice))
        v1  = ws.get_version("1")
        reg = v1.register_request
        
        rootpath = self.discoveryws.rootpath
        virtualpath = rootpath + (webservice.WSPathStaticString("controlpanel"),)
        
        # /ws.v1/netic/OF_UoR/controlpanel/res
        respath=virtualpath+(webservice.WSPathStaticString("res"),)
        reg(self._getres,"GET",respath,"""Get a list of all resources.""","""Administration Sub-Module""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/res/{res_path}
        getrespath= respath + (WSPathExistingRes(self.manager),)
        
        # /ws.v1/netic/OF_UoR/controlpanel/res/{res_path}/caps 
        caprespath=getrespath + (webservice.WSPathStaticString("caps"),)      
        reg(self._rescaps,"GET",caprespath,"""Get a list of all capabilities needed to access a certain resource. (Use : instead of / )""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/res/{res_path}/nocaps
        nocaprespath= respath + (WSPathExistingRes(self.manager),) 
        nocaprespath=getrespath + (webservice.WSPathStaticString("nocaps"),)      
        reg(self._resnocaps,"GET",nocaprespath,"""Get a list of all capabilities not needed to access a certain resource. (Use : instead of / )""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/res/{res_path}/caps/{cap_name}
        addrescappath=caprespath + (WSPathExistingCap(self.manager),)       
        reg(self._addrescap,"POST",addrescappath,"""Add a capaibility to a resource set.""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/res/{res_path}/caps/{cap_name}
        delrescappath=caprespath + (WSPathExistingCap(self.manager),)       
        reg(self._delrescap,"DELETE",delrescappath,"""Delete a capability from a resource set.""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/role
        rolepath=virtualpath + (webservice.WSPathStaticString("role"),)
        reg(self._getAllRoles,"GET", rolepath,"""Get all available roles.""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/editablesrole
        editRolePath = virtualpath + (webservice.WSPathStaticString("editableroles"),)
        reg(self._getAllEditables,"GET", editRolePath,"""Get all available editable roles.""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/editablesrole/caps
        editablesCapPath=editRolePath + (webservice.WSPathStaticString("caps"),)                             
        reg(self._getEditablesCaps,"GET", editablesCapPath,"""Get a list of all capabilities of editable roles.""")

        # /ws.v1/netic/OF_UoR/controlpanel/role/{role_ID}                           
        roleidpath=rolepath + (WSPathExistingRole(self.manager),)
        reg(self._deleterole,"DELETE", roleidpath,"""Delete an existing editable role.""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/role/{role_ID}/caps
        rolecapspath=roleidpath + (webservice.WSPathStaticString("caps"),) 
        reg(self._rolecaps,"GET", rolecapspath,"""Get a list of all capabilites owned by a Role.""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/role/{role_ID}/nocaps
        rolenocapspath=roleidpath + (webservice.WSPathStaticString("nocaps"),)                             
        reg(self._rolenocaps,"GET", rolenocapspath,"""Get a list of all capabilities not owned by a Role.""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/role/{role_ID}/caps/{cap_name}
        addcappath=rolecapspath + (WSPathExistingCap(self.manager),)
        reg(self._addrolecap,"POST", addcappath,"""Add a capability to an editable Role set.""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/role/{role_ID}/caps/{cap_name}
        delcappath=rolecapspath + (WSPathExistingCap(self.manager),)
        reg(self._delrolecap,"DELETE", delcappath,"""Delete a capability from an editable Role set.""")

        # /ws.v1/netic/OF_UoR/controlpanel/role/create/{new_role}
        createrolepath=rolepath + (webservice.WSPathStaticString("create"),)
        addrolepath=createrolepath + (WSPathNewRole(self.manager),)                              
        reg(self._addrole,"POST", addrolepath,"""Add a new editable role.""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/caps
        capspath=virtualpath + (webservice.WSPathStaticString("caps"),)                             
        reg(self._getcaps,"GET", capspath,"""Get a list of all capabilities.""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/user
        userp=virtualpath + (webservice.WSPathStaticString("user"),)                                                      
        reg(self._getusers,"GET", userp,"""Get a list of all users.""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/userroles
        userpath=virtualpath + (webservice.WSPathStaticString("userroles"),)                                                      
        reg(self._getAllUserRoles,"GET", userpath,"""Get a list of all user roles.""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/user/{username}
        usernamepath=userp + (WSPathExistingUser(self.manager),)
        reg(self._deleteuser,"DELETE", usernamepath,"""Delete a user.""")                                                      
        
        # /ws.v1/netic/OF_UoR/controlpanel/user/{username}/roles
        userrolepath=usernamepath + (webservice.WSPathStaticString("roles"),)                             
        reg(self._userroles,"GET", userrolepath,"""Get a list of all roles possessed by a user.""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/getusers/{username}/noroles
        usernorolepath=usernamepath + (webservice.WSPathStaticString("noroles"),)                             
        reg(self._usernoroles,"GET", usernorolepath,"""Get a list of all roles not possessed by a user.""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/user/{username}/roles/{role_ID}
        addroleuserpath=userrolepath + (WSPathExistingRoleAll(self.manager),)                           
        reg(self._addroleuser,"POST", addroleuserpath,"""Add a role to a user's set.""")
        
        # /ws.v1/netic/OF_UoR/controlpanel/user/{username}/roles/{role_ID}
        delroleuserpath=userrolepath + (WSPathExistingRoleAll(self.manager),)                             
        reg(self._delroleuser,"DELETE", delroleuserpath,"""Delete a role from a user's set.""")
        
        
    def _rolecaps(self,request,arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        caps=self.manager.get_cap_by_role(arg["{role_ID}"])
        d={}
        d["caps"]=caps
        neticResponse(request,NTC_OK,d)
        
    def _rolenocaps(self,request,arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        caps=self.manager.get_other_cap_by_role(arg["{role_ID}"])
        d={}
        d["caps"]=caps
        neticResponse(request,NTC_OK,d)
        
    def _getcaps(self,request,arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        caps=self.manager.call_cap_db()
        d={}
        d["caps"]=caps
        neticResponse(request,NTC_OK,d)
        
    def _getEditablesCaps(self,request,arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        roles=self.manager.call_editables_caps_db()
        
        for r in roles:
            caps = r['Caps'].rsplit(',')
            r['Caps'] = caps
        d={}
        d["roles"]=roles
        neticResponse(request,NTC_OK,d)

    def _getAllRoles(self, request, arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        roles=self.manager.get_all_roles_db(False)
        d={}
        d["roles"]=roles
        neticResponse(request,NTC_OK,d)

    def _getAllEditables(self, request, arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        roles=self.manager.get_all_roles_db(True)
        d={}
        d["roles"]=roles
        neticResponse(request,NTC_OK,d)
       
    def _getusers(self,request,arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        users=self.manager.call_users_db()
        d={}
        d["users"]=users
        neticResponse(request,NTC_OK,d)

    def _getAllUserRoles(self,request,arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        users=self.manager.call_all_user_roles_db()
        for user in users:
            #convert from string into list
            roles = user['Roles'].rsplit(',')
            user['Roles'] = roles
        d={}
        d["user_roles"]=users
        neticResponse(request,NTC_OK,d)

    def _getres(self, request, arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        res=self.manager.get_res()
        for r in res:
            caps = r['Caps'].rsplit(',')
            r['Caps'] = caps
            ids = r['IDs'].rsplit(',')
            r['IDs'] = ids
        d={}
        d["res"]=res
        neticResponse(request,NTC_OK,d)
        
    def _rescaps(self, request, arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        caps=self.manager.get_cap_by_res(arg["{res_path}"])
        d={}
        d["caps"]=caps
        neticResponse(request,NTC_OK,d)
        
    def _resnocaps(self, request, arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        caps=self.manager.get_other_cap_by_res(arg["{res_path}"])
        d={}
        d["caps"]=caps
        neticResponse(request,NTC_OK,d)
        
    def _addrescap(self, request, arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        res=arg["{res_path}"]
        cap=arg["{cap_name}"]
        check=self.manager.add_cap_to_res(res,cap)
        d={}
        d["res"]=res
        d["newcap"]=cap
        if check==True:
            d["action"]="Selected capability added to the selected resource"
            d["result"]="SUCCESS"
        else:
            d["action"]="Selected capability already in the resource set"
            d["result"]="FAILURE"
        request.write(simplejson.dumps(d, indent=4))
        request.finish()
        
    def _delrescap(self, request, arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        cap=arg["{cap_name}"]
        res=arg["{res_path}"]
        res=res.replace(":","/")
        check=self.manager.del_cap_from_res(res,cap)
        d={}
        d["res"]=res
        d["cap"]=cap
        if check==True:
            d["action"]="Selected capability deleted from the selected resource set"
            d["result"]="SUCCESS"
        else:
            d["action"]="Selected capability not in the selected resource set"
            d["result"]="FAILURE"
        request.write(simplejson.dumps(d, indent=4))
        request.finish()
        
    def _deleterole(self, request, arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        role=arg["{role_ID}"]
        self.manager.remove_role(role)
        d={}
        d["result"]="User succesfully removed"
        d["role"]=role
        neticResponse(request,NTC_OK,d)
        
    def _addrole(self, request, arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        role=arg["{new_role}"]
        check=self.manager.add_new_role(role)
        d={}
        d["role"]=role
        if check==True:
            d["default capability"]="GET"
            d["action"]="New editable role added to the database"
            d["result"]="SUCCESS"
        else:
            d["action"]="Editable role with this name already stored in the database"
            d["result"]="FAILURE"
        request.write(simplejson.dumps(d, indent=4))
        request.finish()
        
    def _addrolecap(self, request, arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        role=arg["{role_ID}"]
        cap=arg["{cap_name}"]
        check=self.manager.add_cap_to_role(role,cap)
        d={}
        d["role"]=role
        d["newcap"]=cap
        if check==True:
            d["action"]="Selected capability added to the selected editable role"
            d["result"]="SUCCESS"
        else:
            d["action"]="Selected capability already possesed by the selected role"
            d["result"]="FAILURE"
        request.write(simplejson.dumps(d, indent=4))
        request.finish()
        
    def _delrolecap(self, request, arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        role=arg["{role_ID}"]
        cap=arg["{cap_name}"]
        check=self.manager.del_cap_from_role(role,cap)
        d={}
        d["role"]=role
        d["cap"]=cap
        if check==True:
            d["action"]="Selected capability deleted from the selected editable role"
            d["result"]="SUCCESS"
        else:
            d["action"]="Selected capability not possesed by the selected role"
            d["result"]="FAILURE"
        request.write(simplejson.dumps(d, indent=4))
        request.finish()
        
    def _addroleuser(self, request, arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        user=arg["{username}"]
        role=arg["{role_ID}"]
        check=self.manager.add_role_to_user(user,role)
        d={}
        d["user"]=user
        d["role"]=role
        if check==True:
            d["action"]="Selected role added to the selected user"
            d["result"]="SUCCESS"
        else:
            d["action"]="Selected role already possesed by the selected user"
            d["result"]="FAILURE"
        request.write(simplejson.dumps(d, indent=4))
        request.finish()
    
    def _delroleuser(self, request, arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        user=arg["{username}"]
        role=arg["{role_ID}"]
        check=self.manager.del_role_from_user(user,role)
        d={}
        d["user"]=user
        d["role"]=role
        if check==True:
            d["action"]="Selected role deleted from the selected user"
            d["result"]="SUCCESS"
        else:
            d["action"]="Selected role not possesed by the selected user"
            d["result"]="FAILURE"
        request.write(simplejson.dumps(d, indent=4))
        request.finish()
        
    def _userroles(self, request, arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        roles=self.manager.get_role_by_user(arg["{username}"])
        d={}
        d["roles"]=roles
        neticResponse(request,NTC_OK,d)
        
    def _usernoroles(self, request, arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        roles=self.manager.get_other_role_by_user(arg["{username}"])
        d={}
        d["roles"]=roles
        neticResponse(request,NTC_OK,d)
        
    def _deleteuser(self, request, arg):
        request.setResponseCode(200)
        request.setHeader("Content-Type", "application/json")
        user=arg["{username}"]
        self.manager.remove_user(user)
        d={}
        d["result"]="User succesfully removed"
        d["user"]=user
        neticResponse(request,NTC_OK,d)
                
    def getInterface(self):
        return str(ControlTools)

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return ControlTools(ctxt)

    return Factory()
    
class WSPathExistingUser(webservice.WSPathComponent):
        def __init__(self, manager):
            webservice.WSPathComponent.__init__(self)
            self._manager = manager
            
        def __str__(self):
            return "{username}"

        def extract(self, user_str, data):
            if self._manager.check_user(user_str):
                return webservice.WSPathExtractResult(value=user_str)
            else:
                e = "'%s' is not a valid user name." % user_str
                return webservice.WSPathExtractResult(error=e)
                
class WSPathExistingRole(webservice.WSPathComponent):
        def __init__(self, manager):
            webservice.WSPathComponent.__init__(self)
            self._manager = manager
            
        def __str__(self):
            return "{role_ID}"

        def extract(self, role_str, data):
            if self._manager.check_role(role_str):
                return webservice.WSPathExtractResult(value=role_str)
            else:
                e = "'%s' is not a valid role_ID." % role_str
                return webservice.WSPathExtractResult(error=e)
                
class WSPathExistingRoleAll(webservice.WSPathComponent):
        def __init__(self, manager):
            webservice.WSPathComponent.__init__(self)
            self._manager = manager
            
        def __str__(self):
            return "{role_ID}"

        def extract(self, role_str, data):
            if self._manager.check_role_all(role_str):
                return webservice.WSPathExtractResult(value=role_str)
            else:
                e = "'%s' is not a valid role_ID." % role_str
                return webservice.WSPathExtractResult(error=e)
                
class WSPathNewRole(webservice.WSPathComponent):
        def __init__(self, manager):
            webservice.WSPathComponent.__init__(self)
            self._manager = manager
            
        def __str__(self):
            return "{new_role}"

        def extract(self, role_str, data):
            if self._manager.check_role(role_str):
                e = "'%s' is already a valid role_ID." % role_str
                return webservice.WSPathExtractResult(error=e)
            else:
                return webservice.WSPathExtractResult(value=role_str)
                
                
class WSPathNewCap(webservice.WSPathComponent):
        def __init__(self, manager):
            webservice.WSPathComponent.__init__(self)
            self._manager = manager
            
        def __str__(self):
            return "{new_cap}"

        def extract(self, cap_str, data):
            if self._manager.check_cap(cap_str):
                e = "'%s' is already valid capability name." % cap_str
                return webservice.WSPathExtractResult(error=e)
            else:
                return webservice.WSPathExtractResult(value=cap_str)    
                
class WSPathExistingCap(webservice.WSPathComponent):
        def __init__(self, manager):
            webservice.WSPathComponent.__init__(self)
            self._manager = manager
            
        def __str__(self):
            return "{cap_name}"

        def extract(self, cap_str, data):
            if self._manager.check_cap(cap_str):
                return webservice.WSPathExtractResult(value=cap_str)
            else:
                e = "'%s' is not a valid capability name." % cap_str
                return webservice.WSPathExtractResult(error=e)
                
class WSPathExistingRes(webservice.WSPathComponent):
        def __init__(self, manager):
            webservice.WSPathComponent.__init__(self)
            self._manager = manager
            
        def __str__(self):
            return "{res_path}"

        def extract(self, res_str, data):
            res_str=res_str.replace(":","/")
            if self._manager.check_res(res_str):
                return webservice.WSPathExtractResult(value=res_str)
            else:
                e = "'%s' is not a valid resource path." % res_str
                return webservice.WSPathExtractResult(error=e)
