import io
import socket
import time
from PIL import Image

paths = ["/home/jatin/Pictures/disp/img1.jpg","/home/jatin/Pictures/disp/img2.jpg",
        "/home/jatin/Pictures/disp/img3.jpg","/home/jatin/Pictures/disp/img4.jpg",
        "/home/jatin/Pictures/disp/img5.jpg" ]   

serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# code block for reusable address of port for serving  #

old_state = serv.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR) 
serv.setsockopt( socket.SOL_SOCKET, socket.SO_REUSEADDR, 1 ) # Enable the SO_REUSEADDR option
new_state = serv.getsockopt( socket.SOL_SOCKET, socket.SO_REUSEADDR )

# code block ended #

serv.bind(('0.0.0.0', 8090))
serv.listen(5)
print("Ready to accept 5 connections")


def create_bytes_from_image(path):
    img = Image.open(path, mode='r')

    img_byte_arr = io.BytesIO()
    img.save(img_byte_arr, format='JPEG')
    img_byte_arr = img_byte_arr.getvalue()
    return img_byte_arr


while True:
    conn, addr = serv.accept()
    #array_from_client = bytearray()
    start = time.time()
    

    while True:
        shape = None
        string_rec=''
        data = conn.recv(4096)
        if not data :  # no data received
            break
        elif shape is None:
            string_rec += data.decode("utf-8")
            print("client sent ", string_rec)
            if (string_rec == "S"):
                conn.sendall(create_bytes_from_image(paths[1]))   # sends image from JPEG
            if (string_rec == "S1"):
                conn.sendall(create_bytes_from_image(paths[1]))   # sends image from JPEG
            if (string_rec == "S2"):
                conn.sendall(create_bytes_from_image(paths[0]))   # sends image from JPEG
            if (string_rec == "S3"):
                conn.sendall(create_bytes_from_image(paths[4]))   # sends image from JPEG
        '''        
        try:
            print("sending image from jpeg")
            time.sleep(5)
            for i in paths:
                print("sent",i)
                conn.sendall(create_bytes_from_image(i))   # sends image from JPEG
                time.sleep(3)
        except:
            pass
        '''
    
    while True:
        data = conn.recv(4096)
        if not data :  # no data received
            break

    #conn.close()
    #print('client disconnected')
