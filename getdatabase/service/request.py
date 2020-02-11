#This program is for web-scrapping

import requests
import os
from urllib import parse

os.system("chcp 949")

print("[+] Startup Downloader")

Url = "http://www.ftc.go.kr/www/dataOpen.do?key=257"


Response = requests.get(Url)

FindFirstLocationNumberString = Response.text


def FindFirstLocationNumber(Init):

    lpLocation = Init
    if(lpLocation in Response.text):
        #print("[+] Successfully find locations!\n")

        #find location
        LocationIndex = Response.text.find(lpLocation)
        ResponseStringLength = len(Response.text)

        OptionalLocationIndex = Response.text[LocationIndex: ResponseStringLength].find(
            '<option value="')

    #####
    LocationIndex = 0

    LocationIndex = Response.text.find(lpLocation)
    ResponseStringLength = len(Response.text)

    OptionalLocationIndex = Response.text[LocationIndex: ResponseStringLength].find(
        '<option value="')

    lpFind = Response.text[LocationIndex +
                           OptionalLocationIndex: ResponseStringLength]
    LocationIndex += lpFind.find("\r\n")

    lpFind = Response.text[LocationIndex +
                           OptionalLocationIndex: ResponseStringLength]
    LocationIndex += lpFind.find('<option value="')

    lpFind = Response.text[LocationIndex +
                           OptionalLocationIndex: ResponseStringLength]

    OptionalValueLength = len('<option value="')
    lpFindLength = len(lpFind)

    lpFind = lpFind[OptionalValueLength: lpFindLength]

    LocationNumber = ""

    i = 0
    while(lpFind[i] != '"'):
        LocationNumber += lpFind[i]
        i += 1

    #print(LocationNumber)

    global FindFirstLocationNumberString
    FindFirstLocationNumberString = lpFind

    return lpFind


def FindNextLocationNumber():
    global FindFirstLocationNumberString
    CrlfFind = FindFirstLocationNumberString.find("\r\n")
    lpFind = FindFirstLocationNumberString[CrlfFind: len(FindFirstLocationNumberString)]

    FindFirstLocationNumberString = lpFind

    OptionalLocationIndex = lpFind.find('<option value="')
    if(OptionalLocationIndex == 0):
        return None

    lpFind = lpFind[OptionalLocationIndex +
        len('<option value="'): len(lpFind)]
    FindFirstLocationNumberString = lpFind

    LocationNumber = ""

    i = 0
    while(True):
        if(i == 7):
            break

        if(lpFind[i] == '"'):
            break

        LocationNumber += lpFind[i]
        i += 1

    if(i != 7):
        return None

    return LocationNumber



f = open("links.txt", "w", encoding="UTF-8")
f.close()

lpLocationNumber = FindFirstLocationNumber('<select name="area1" id="area1" title=')

Seoul = ""
i = 0
while(i != 7):
    Seoul += lpLocationNumber[i]
    i += 1


PostData = {"schCheck": "", 
            "menu_id": "01", 
            "area1": Seoul,
            "area2": ""}

Response = requests.post(Url, params=PostData)
    

f = open("htmls/tmp.html", "w", encoding="UTF-8")
f.write(Response.text)
f.close()

os.system("py _downloader.py")

while(True):
    lpLocationNumber = FindNextLocationNumber()
    if(lpLocationNumber == None):
        break

    PostData = {"schCheck": "", 
            "menu_id": "01", 
            "area1": lpLocationNumber,
            "area2": ""}

    Response = requests.post(Url, params=PostData)
    

    f = open("htmls/tmp.html", "w", encoding="UTF-8")
    f.write(Response.text)
    f.close()

    os.system("py _downloader.py")

os.system("get.exe")