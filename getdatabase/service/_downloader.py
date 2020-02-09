import threading
import re
import sys
import requests
from urllib import parse 
from collections import namedtuple

def ReturnLocationNumber(Str):
    ret = ""
    i = 0

    while(i != 7):
        ret += Str[i]
        i += 1

    return ret

def FindFirstLocationName():
    f = open("htmls/tmp.html", "r", encoding="UTF-8")
    FindFirstLocationNumberString = f.read()
    f.close()
    
    target = '<select name="area2" id="area2" title='
    index = FindFirstLocationNumberString.find(target)
    lpFind = FindFirstLocationNumberString[index: len(FindFirstLocationNumberString)]
    
    _target = '<option value="'

    index = lpFind.find(_target)
    lpFind = lpFind[index + len(_target) : len(lpFind)]

    return lpFind


FindIndex = 0

def FindNextLocationNumber(lpFind):
    global FindIndex 
    FindIndex += 1

    i = 0

    while(i != FindIndex):
        _target = '<option value="'
        index = lpFind.find(_target)
        lpFind = lpFind[index + len(_target) : len(lpFind)]
        i += 1
    
    return lpFind


def FindNextLocationName(lpFind):
    
    global FindIndex 

    i = 0

    while(i != FindIndex):
        _target = '<option value="'
        index = lpFind.find(_target)
        lpFind = lpFind[index + len(_target) + 9 : len(lpFind)]
        i += 1

    ret = ""

    i = 0
    while(True):
        if(lpFind[i] == '<'):
            break
        
        ret += lpFind[i]

        i += 1
    
    return ret

def FindCurrentCityName():
    f = open("htmls/tmp.html", "r", encoding="UTF-8")
    FindFirstLocationNumberString = f.read()
    f.close()

    IsSelected = 'selected="selected"'
    SelectedLength = FindFirstLocationNumberString.find(IsSelected)
    lpFind = FindFirstLocationNumberString[SelectedLength: len(FindFirstLocationNumberString)]
    SelectedLength = lpFind.find(">")

    lpFind = lpFind[SelectedLength + 1: len(lpFind)]

    CityName = ""

    i = 0
    while(True):
        if(lpFind[i] == '<'):
            break

        CityName += lpFind[i]
        i += 1

    return CityName

CityName = FindCurrentCityName()
data = FindFirstLocationName()
FindNextLocationName(data)

Url = 'http://www.ftc.go.kr/www/cmm/fms/FileWorkDown.do?atchFileUrl=/dataopen&atchFileNm='

f = open("links.txt", "a", encoding="UTF-8")

while(True):
    number = ReturnLocationNumber(FindNextLocationNumber(data))
    name = FindNextLocationName(data)
    
    try :
        int(number) 
    except ValueError :
        break
        
    RequestParameter = "통신판매사업자_" + number + "_" + CityName + " " + name + ".csv"
    print(RequestParameter)

    DownloadUrl = parse.quote(RequestParameter)
    DownloadUrl = parse.quote(DownloadUrl)

    f.write(Url + DownloadUrl)
    f.write("\r\n")

f.close()