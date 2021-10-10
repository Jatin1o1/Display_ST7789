import io
import socket
import time

import numpy as np
from PIL import Image



serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)



"""  code block for reusable address of port for serving  """
# Get the old state of the SO_REUSEADDR option
old_state = serv.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR) 

# Enable the SO_REUSEADDR option
serv.setsockopt( socket.SOL_SOCKET, socket.SO_REUSEADDR, 1 )
new_state = serv.getsockopt( socket.SOL_SOCKET, socket.SO_REUSEADDR )
#print ("New sock state:",new_state)

""" code block ended """


serv.bind(('0.0.0.0', 8090))
serv.listen(2)
print("Ready to accept 5 connections")


def create_image_from_bytes(image_bytes) -> Image.Image:
    stream = io.BytesIO(image_bytes)
    return Image.open(stream)


while True:
    conn, addr = serv.accept()
    array_from_client = bytearray()
    shape = None
    chunks_received = 0
    start = time.time()
    shape_string = ''
    while True:
        # print('waiting for data')
        # Try 4096 if unsure what buffer size to use. Large transfer chunk sizes (which require large buffers) can cause corrupted results
        data = conn.recv(65535)
        if not data or data == b'tx_complete':
            break
        elif shape is None:
            shape_string += data.decode("utf-8")
            # Find the end of the line.  An index other than -1 will be returned if the end has been found because 
            # it has been received
            if shape_string.find('\r\n') != -1:
                width_index = shape_string.find('width:')
                height_index = shape_string.find('height:')
                width = int(shape_string[width_index + len('width:'): height_index])
                height = int(shape_string[height_index + len('height:'): ])
                shape = (width, height)
            print("shape is {}".format(shape))
            print("data is {}".format(data.decode("utf-8")))
            
        else:
            chunks_received += 1
            # print(chunks_received)
            array_from_client.extend(data)
            print("data is {}".format(data.decode("utf-8")))
            # print(array_from_client)
        conn.sendall(b'ack')
        # print("sent acknowledgement")
    #     TODO: need to check if sending acknowledgement of the number of chunks and the total length of the array is a good idea
    print("chunks_received {}. Number of bytes {}".format(chunks_received, len(array_from_client)))
    img: Image.Image = create_image_from_bytes(array_from_client)
    img.show()
    array_start_time = time.time()
    image_array = np.asarray(img)
    print('array conversion took {} s'.format(time.time() - array_start_time))
    conn.close()
    print('client disconnected')
