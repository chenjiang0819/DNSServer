"""Only loopback transport and a local deterministic DNS peer; no external DNS."""
from pathlib import Path
import os,struct,socket,socketserver,threading,time,subprocess,unittest,tempfile,concurrent.futures
ROOT=Path(__file__).resolve().parents[1]
PROGRAM=Path(os.environ.get('PROGRAM',ROOT/'dns_svr'))
def packet(domain='example.test',ident=1,qtype=28):
 qname=b''.join(bytes([len(part)])+part.encode() for part in domain.split('.'))+b'\0'
 body=struct.pack('!6H',ident,0x100,1,0,0,0)+qname+struct.pack('!HH',qtype,1)
 return struct.pack('!H',len(body))+body

def exact(s,n):
 b=b''
 while len(b)<n:
  part=s.recv(n-len(b))
  if not part:raise EOFError()
  b+=part
 return b

def frame(s):
 prefix=exact(s,2);return prefix+exact(s,struct.unpack('!H',prefix)[0])

def name(p):
 pos=14;parts=[]
 while p[pos]:n=p[pos];parts.append(p[pos+1:pos+1+n].decode());pos+=n+1
 return '.'.join(parts)

class Peer(socketserver.ThreadingTCPServer):
 allow_reuse_address=True;daemon_threads=True;request_queue_size=128
class Handler(socketserver.BaseRequestHandler):
 def handle(self):
  self.request.settimeout(3)
  try:
   q=frame(self.request);domain=name(q)
   with self.server.lock:self.server.counts[domain]=self.server.counts.get(domain,0)+1
   if domain.startswith('slow'):time.sleep(2.4)
   if domain.startswith('eof'):return
   if domain.startswith('malformed'):
    self.request.sendall(b'\0\x0c'+b'\0'*12);return
   ttl=1 if domain.startswith('expires') else 700
   body=bytearray(q[2:]);struct.pack_into('!H',body,2,0x8180);struct.pack_into('!H',body,6,1)
   if domain.startswith('wrong'):struct.pack_into('!H',body,0,65530)
   answer=b'\xc0\x0c'+struct.pack('!HHIH',28,1,ttl,16)+socket.inet_pton(socket.AF_INET6,'2001:db8::1')
   body+=answer;response=struct.pack('!H',len(body))+body
   # A peer can also segment both prefix and payload.
   for part in [response[:1],response[1:3],response[3:]]:self.request.sendall(part)
  except (OSError,EOFError):pass

class ProtocolTests(unittest.TestCase):
 def setUp(self):
  self.peer=Peer(('127.0.0.1',0),Handler);self.peer.counts={};self.peer.lock=threading.Lock()
  self.thread=threading.Thread(target=self.peer.serve_forever,daemon=True);self.thread.start()
  with socket.socket() as s:s.bind(('127.0.0.1',0));self.port=s.getsockname()[1]
  build=ROOT/'.build';build.mkdir(exist_ok=True)
  self.tmp=tempfile.TemporaryDirectory(dir=build);self.errors=open(Path(self.tmp.name)/'stderr.txt','w+')
  env=dict(os.environ,DNS_BIND_ADDRESS='127.0.0.1')
  self.process=subprocess.Popen([str(PROGRAM),'127.0.0.1',str(self.peer.server_address[1]),str(self.port)],cwd=self.tmp.name,env=env,stdout=subprocess.DEVNULL,stderr=self.errors)
  for _ in range(150):
   if self.process.poll() is not None:self.fail('server exited at startup')
   try:
    with self.connect():break
   except OSError:time.sleep(.02)
  else:self.fail('server did not listen')
 def tearDown(self):
  self.process.terminate()
  try:self.process.wait(timeout=7)
  except subprocess.TimeoutExpired:self.process.kill();self.process.wait();self.fail('graceful stop timed out')
  self.errors.seek(0);errors=self.errors.read();code=self.process.returncode;self.errors.close()
  self.peer.shutdown();self.peer.server_close();self.thread.join();self.tmp.cleanup()
  self.assertEqual(code,0,errors);self.assertNotIn('Sanitizer',errors);self.assertEqual(errors,'')
 def connect(self):return socket.create_connection(('127.0.0.1',self.port),3)
 def ask(self,q,fragment=False):
  with self.connect() as s:
   if fragment:
    for byte in q:s.sendall(bytes([byte]));time.sleep(.001)
   else:s.sendall(q)
   return frame(s)
 def test_fragmented_frames_aaaa_and_unimplemented(self):
  q=packet(ident=258);r=self.ask(q,True);self.assertEqual(r[2:4],q[2:4]);self.assertEqual(name(r),'example.test');self.assertEqual(r[-16:],socket.inet_pton(socket.AF_INET6,'2001:db8::1'))
  q=packet(qtype=1);r=self.ask(q);self.assertEqual(r[5]&15,4);self.assertTrue(r[4]&128);self.assertEqual(len(r),len(q))
 def test_cache_ids_ttl_expiry_and_eviction(self):
  self.ask(packet('cached.test',1));time.sleep(1.1)
  first=self.ask(packet('cached.test',2));second=self.ask(packet('cached.test',3))
  ttl=lambda r:struct.unpack('!I',r[-22:-18])[0]
  self.assertTrue(697<=ttl(first)<700);self.assertLessEqual(ttl(second),ttl(first));self.assertGreaterEqual(ttl(second),ttl(first)-1)
  self.assertEqual(second[2:4],b'\0\3');self.assertEqual(self.peer.counts['cached.test'],1)
  self.ask(packet('expires.test'));time.sleep(1.2);self.ask(packet('expires.test'));self.assertEqual(self.peer.counts['expires.test'],2)
  for i in range(7):self.ask(packet(f'evict{i}.test'))
  self.ask(packet('evict0.test'));self.assertEqual(self.peer.counts['evict0.test'],2)
 def test_malformed_eof_and_total_deadline(self):
  q=packet();cases=[b'',b'\0',b'\0\2\0\0',b'\0\x0c'+b'\0'*12,q[:-1],q[:14]+b'\xff'+q[15:]]
  loop=bytearray(q);loop[14:16]=b'\xc0\x0c';cases.append(bytes(loop))
  for bad in cases:
   with self.connect() as s:
    s.sendall(bad);s.shutdown(socket.SHUT_WR)
    try:self.assertEqual(s.recv(100),b'')
    except ConnectionResetError:pass
  # Prefix arrives slowly but must still close by the absolute deadline.
  with self.connect() as s:
   start=time.monotonic();s.sendall(q[:1]);time.sleep(1);s.sendall(q[1:2]);self.assertEqual(s.recv(100),b'');self.assertLess(time.monotonic()-start,2.8)
  self.assertEqual(name(self.ask(packet())),'example.test')
 def test_failed_upstream_is_isolated(self):
  for domain in ['wrong.test','eof.test','malformed.test','slow.test']:
   start=time.monotonic()
   with self.connect() as s:
    s.sendall(packet(domain));self.assertEqual(s.recv(100),b'')
   self.assertLess(time.monotonic()-start,2.8)
   self.assertEqual(name(self.ask(packet('still-alive.test'))),'still-alive.test')
 def test_concurrent_cache_and_request_ownership(self):
  def run(i):
   domain=f'parallel{i%11}.test';q=packet(domain,i+1);r=self.ask(q)
   self.assertEqual(r[2:4],q[2:4]);self.assertEqual(name(r),domain);self.assertEqual(r[-16:],socket.inet_pton(socket.AF_INET6,'2001:db8::1'))
  with concurrent.futures.ThreadPoolExecutor(max_workers=32) as pool:list(pool.map(run,range(240)))
  self.assertIsNone(self.process.poll())
 def test_invalid_arguments(self):
  for args in [[],['127.0.0.1','0'],['127.0.0.1','65536'],['127.0.0.1','x']]:
   r=subprocess.run([str(PROGRAM),*args],cwd=self.tmp.name,capture_output=True,timeout=3);self.assertNotEqual(r.returncode,0)
if __name__=='__main__':unittest.main()
