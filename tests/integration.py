import socket,subprocess,signal,time,struct,json,http.client
import argparse
parser = argparse.ArgumentParser(description="HTTP / WebSocket 核心联调")
parser.add_argument('--server', required=True)
parser.add_argument('--client-smoke')
args = parser.parse_args()
exe = args.server
def start():
 p=subprocess.Popen([exe],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
 assert '已启动' in p.stdout.readline()
 return p
def ws():
 s=socket.create_connection(('127.0.0.1',8080));s.settimeout(4)
 s.sendall(b'GET /ws HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n')
 r=b''
 while b'\r\n\r\n' not in r:r+=s.recv(1)
 assert b'101' in r
 return s
def exact(s,n):
 r=b''
 while len(r)<n:
  v=s.recv(n-len(r));assert v;r+=v
 return r
def echo(s,op,b):
 mask=b'abcd';s.sendall(bytes([128|op,128|len(b)])+mask+bytes(v^mask[i%4] for i,v in enumerate(b)))
 h=exact(s,2);assert h[0]==128|op;assert exact(s,h[1]&127)==b

def post(value):
 c=http.client.HTTPConnection('127.0.0.1',8080,timeout=3)
 c.request('POST','/api/echo',json.dumps(value,ensure_ascii=False).encode())
 r=c.getresponse(); assert r.status==200; assert json.loads(r.read())=={'data':value}; c.close()

def receive(s):
 h=exact(s,2); n=h[1]&127
 if n==126:n=struct.unpack('!H',exact(s,2))[0]
 elif n==127:n=struct.unpack('!Q',exact(s,8))[0]
 return h[0]&15,exact(s,n)

for sig in [signal.SIGINT,signal.SIGTERM]:
 p=start(); connections=[]
 try:
  post({'无订阅者':True})
  connections=[ws(),ws()]
  for value in [{'中文':'你好\n"'},[1,None],42,True,None,'text']:
   post(value)
   for s in connections:
    op,data=receive(s); assert op==1 and json.loads(data)==value
  c=http.client.HTTPConnection('127.0.0.1',8080,timeout=3)
  c.request('POST','/api/echo',b'invalid');r=c.getresponse();assert r.status==400;r.read();c.close()
  s=connections[0];s.settimeout(.5)
  try:s.recv(1);raise AssertionError('非法请求产生广播')
  except socket.timeout:pass
  s.sendall(bytes([129,128])+b'abcd')
  try:s.recv(1);raise AssertionError('WS 业务消息被回显')
  except socket.timeout:pass
  s.close();connections.remove(s);connections.append(ws())
  post({'reconnect':True})
  for s in connections:assert json.loads(receive(s)[1])=={'reconnect':True}
  for s in connections:s.close()
  connections=[]
  if args.client_smoke:subprocess.run([args.client_smoke],check=True,timeout=25)
  connections=[ws()]
  p.send_signal(sig);out,err=p.communicate(timeout=5)
  assert p.returncode==0 and '已回收' in out,(out,err)
  print(sig.name,'广播、重连与正常停止通过')
 finally:
  for s in connections:s.close()
  if p.poll() is None:p.kill();p.wait()
p=start()
p.send_signal(signal.SIGTERM);p.wait(timeout=5)
assert p.returncode==0
print('端口重新绑定通过，测试进程已清理。')
