#import heapq
import Queue

f = open("var_time_info",'r')
hashF = open("HashValues", 'w')
sizeF = open("VarMinSize", 'w')

lineVar = f.readlines()[-3]
countVar = int(lineVar.split(':')[1])
f.close()

f = open("var_time_info",'r')

# In every iteration in for loop, object entry comes into hash queue and go out to size queue
hashObj = Queue.Queue()
sizeObj = Queue.Queue()
flagHero = dict()  # If an index is set, that index of variable is hero variable
sizeVar = dict()  # Key is variable's hash value

for i in f.readlines():
  if "[Hash]" in i:
    hashVal = int(i.split()[1])
    if not flagHero.has_key(hashVal):
      flagHero[hashVal] = False
      sizeVar[hashVal] = 0
    hashObj.put(hashVal)

  elif "[Trace]" in i and "size" in i:
    tmpHash, tmpSize = -1, -1
    if not hashObj.empty():
      tmpHash = hashObj.get()
    else:
      print "No [Hash] before [Trace]"
      continue
    
    # Parsing [Trace] line's size value
    for j in i.split():
      if len(j.split(':')) > 1:
        if j.split(':')[0] == "size":
          tmpSize = int(j.split(':')[1])

          # Hero variable : The one whose largest object is le than 4096
          if tmpSize > 4095:
            flagHero[tmpHash] = True


f.close()

hashObj_after = Queue.Queue()
sizeObj_after = Queue.Queue()

f = open("var_time_info",'r')

for i in f.readlines():
  if "[Hash]" in i:
    hashVal = int(i.split()[1])
    hashObj_after.put(hashVal)
    #set_hash.add(int(i.split()[1]))
  elif "[Trace]" in i and "size" in i:
    tmpHash, tmpSize = -1, -1
    if not hashObj_after.empty():
      tmpHash = hashObj_after.get()

    else:
      print "No [Hash] before [Trace]"
      continue
    
    for j in i.split():
      if len(j.split(':')) > 1:
        if j.split(':')[0] == "size":
          tmpSize = int(j.split(':')[1])
          sizeObj_after.put((tmpHash, tmpSize))

  tmpTup = ()

  if not sizeObj_after.empty():
    tmpTup = sizeObj_after.get()
    
    if flagHero[tmpTup[0]]:
      hashF.write(str(tmpTup[0])+'\n')
      sizeF.write(str(tmpTup[1])+'\n')


f.close()
hashF.close()
sizeF.close()
