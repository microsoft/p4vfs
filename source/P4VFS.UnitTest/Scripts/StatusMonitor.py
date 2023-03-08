import os
import time
import re
import subprocess

kRootLogFolder = r'\\tc\studio\logs\VFS'
kSplunkLog = False
kSplunkPublishLogFolder = kRootLogFolder+"\\splunk"
kSplunkWorkingLogFolder = kRootLogFolder+"\\splunk_tmp"
kLogTypes = ['error']
kTimeSpan = 24*60*60*10
kTimeMin = time.time() - kTimeSpan

def writeLineConsole(settings, logType, logTime, filePath, line):
    if logTime < kTimeMin:
        return
    if not logType in kLogTypes:
        return
    if not 'header' in settings:
        print('\n%s' % filePath)
        settings['header'] = True
    print('   %s' % line)
    
def writeLineSplunk(settings, logType, logTime, filePath, line):
    if logTime < kTimeMin:
        return
    if not logType in kLogTypes and \
       not '<Info> - Virtual Sync [' in line and \
       not '<Info> - VirtualFileSystem.PopulateFile' in line and \
       not '<Info> - Started at' in line:
        return
    if not 'outFile' in settings:
        fileName = os.path.basename(filePath)
        userName = os.path.abspath(filePath).split('\\')[-4]
        outFile = '%s\\%s\\%s' % (kSplunkWorkingLogFolder, userName, fileName)
        outFolder = os.path.dirname(outFile)
        if not os.path.isdir(outFolder):
            os.makedirs(outFolder)
        if os.path.isfile(outFile):
            os.unlink(outFile)
        settings['outFile'] = outFile
    with open(settings['outFile'], 'at') as file:
        file.write('%s\n' % line)        
    
def monitorLogFile(filePath):
    modifiedTime = os.path.getmtime(filePath)
    if modifiedTime < kTimeMin:
        return
    consoleSettings = {}
    splunkSettings = {}
    with open(filePath, 'r') as file:
        for line in file:
            line = line.rstrip()
            m = re.search(r'^-(?P<mon>\d+)/(?P<day>\d+)/(?P<year>\d{4})-(?P<hour>\d+):(?P<min>\d+):(?P<sec>\d+)(\s(?P<period>AM|PM))?::<(?P<log>\w+)>', line)
            if m == None:
                #print('    $$ %s' % line)
                continue
            logType = m.group('log').lower()
            period = 12 if m.group('period') == 'PM' else 0
            tstruct = time.struct_time([int(m.group('year')),int(m.group('mon')),int(m.group('day')),int(m.group('hour'))+period,int(m.group('min')),int(m.group('sec')),-1,-1,-1])
            logTime = time.mktime(tstruct)
            writeLineConsole(consoleSettings, logType, logTime, filePath, line)
            if kSplunkLog:
                writeLineSplunk(splunkSettings, logType, logTime, filePath, line)
    
def monitorLogFolder(folderPath):
    for fileName in filter(lambda n: not 'splunk' in n, os.listdir(folderPath)):
        filePath = os.path.join(folderPath, fileName)
        if os.path.isdir(filePath):
            monitorLogFolder(filePath)
        else:
            monitorLogFile(filePath)
    
def deleteFolder(folderPath, retryCount=10, retryDelay=1):
    if os.path.isdir(folderPath):
        for retryIndex in range(retryCount):
            print('Removing: "%s"%s' % (folderPath, ' [Retry %d]'%retryIndex if retryIndex else ''))
            subprocess.call('rmdir /s /q "%s"' % folderPath, shell=True)
            if not os.path.isdir(folderPath):
                return
            time.sleep(retryDelay)
        print('Failed to delete: "%s"' % folderPath)
        
def moveFolder(folderPath, newFolderPath, retryCount=10, retryDelay=1):
    deleteFolder(newFolderPath, retryCount, retryDelay)
    if os.path.isdir(folderPath) and not os.path.isdir(newFolderPath):
        for retryIndex in range(retryCount):
            print('Moving: "%s" -> "%s"%s' % (folderPath, newFolderPath, ' [Retry %d]'%retryIndex if retryIndex else ''))
            subprocess.call('move "%s" "%s"' % (folderPath, newFolderPath), shell=True)
            if not os.path.isdir(folderPath) and os.path.isdir(newFolderPath):
                return
            time.sleep(retryDelay)
            
def main(args):
    print("Status from: %s" % time.ctime(kTimeMin))
    deleteFolder(kSplunkWorkingLogFolder)
    monitorLogFolder(kRootLogFolder)
    moveFolder(kSplunkWorkingLogFolder, kSplunkPublishLogFolder)
    
if __name__ == '__main__':
    main(os.sys.argv)
    