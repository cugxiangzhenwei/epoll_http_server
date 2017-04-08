#!/usr/bin/python
#coding=utf-8
import requests
import json

s = requests

dJson={"username":"xiangzhenwei","password":"123456"}
hd = {'Content-Type': 'application/json'}
dJson = json.dumps(dJson);
r = s.post('http://127.0.0.1:5000/login', data = dJson,headers=hd);

print r.status_code
print r.headers['content-type']
print r.encoding
print r.text
