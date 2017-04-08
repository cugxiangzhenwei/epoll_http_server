#!/usr/bin/python
#coding=utf-8
import requests
import json

s = requests

dJson={"username":"xiangzhenwei","password":"123456","confirmpassword":"123456","phone":"12388751151","email":"cugxiangzhenwei@sina.cn","sex":"m","nickname":"博朗科技"}
hd = {'Content-Type': 'application/json'}
dJson = json.dumps(dJson);
r = s.post('http://127.0.0.1:5000/register', data = dJson,headers=hd);

print r.status_code
print r.headers['content-type']
print r.encoding
print r.text
