import pyaudio
import wave
import socket
import sys,os,time

CHUNK = 1024
FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 16000
RECORD_SECONDS = 10
WAVE_OUTPUT_FILENAME = "output.wav"
IP = "58.87.95.166"
PORT=60000

p = pyaudio.PyAudio()

stream = p.open(format=FORMAT,
                channels=CHANNELS,
                rate=RATE,
                input=True,
                frames_per_buffer=CHUNK)

print("* recording")

frames = []




def do_connect(host,port):
    print("try connect to server")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try :
        sock.connect((host,port))
    except :
        pass
    return sock
# # Create a TCP/IP socket
# sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# # Connect the socket to the port where the server is listening
# server_address = (IP, PORT)
# sock.connect(server_address)
def main():
    print("test")
    sock = do_connect(IP, PORT)

    while True:
        try:
            data = stream.read(CHUNK)
            sock.sendall(data)
         #   print("record and sending:%d" % len(data))
        except KeyboardInterrupt:
            pass
        except socket.error:
            print("socket error")
            time.sleep(3)
            sock=do_connect(IP,PORT)
        except :
            print("other error occur")
        #time.sleep(1)
   # frames.append(data)

print("* done recording")

# stream.stop_stream()
# stream.close()
# p.terminate()
# sock.close
if __name__ == "__main__":
    # print("hello")
    
    main()
 
