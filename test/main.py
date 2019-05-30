"""
This module is for simulating the sending of data from the embedded systems
team.
"""

__author__ = "Saveliy Yusufov"
__email__ = "sy2685@columbia.edu"
__version__ = "1.0"

import sys
import socket
import logging
import threading
from multiprocessing import Queue


logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

# Create console handler and set level to debug
ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)

# Create formatter
formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")

# Add formatter to ch
ch.setFormatter(formatter)

# Add ch to logger
logger.addHandler(ch)


def file_reader(filename, queue):
    """Reads a file line by line and puts each line on the queue

    Parameters
    ----------
    filename: str
        The full path to the file to be read-in.

    queue: Queue
        A thread and process safe FIFO queue.

    References
    ----------
    https://docs.python.org/3.4/library/multiprocessing.html#exchanging-objects-between-processes
    """
    logger.info("file_reader thread started!")
    data_file = open(filename, 'r')

    for line in data_file:
        queue.put(line.strip())

    data_file.close()


class Client:
    """Data sender client

    This class is meant to simulate the microcontoller that will connect and
    send data to the device on which the Ultrasound image is reconstructed.
    """

    def __init__(self, address, port, data_queue):
        self.data_queue = data_queue

        # Create a TCP/IP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        logger.info("Client connecting to %s port %s", address, port)

        # Connect the socket to the port where the server is listening
        try:
            self.sock.connect((address, port))
        except socket.error as msg:
            logger.exception(msg)
            sys.exit(1)

        thread = threading.Thread(target=self.data_sender, args=())
        thread.daemon = True
        thread.start()

    def data_sender(self):
        """Send data across the socket"""
        logger.info("data_sender thread started")
        while True:
            value = self.data_queue.get()
            data = "{}|".format(value)
            data = data.encode()
            self.sendall(data)

    def sendall(self, data):
        """Wraps socket module's `sendall` function"""
        try:
            self.sock.sendall(data)
        except socket.error as msg:
            logger.exception(msg)
            self.sock.close()


def main():
    """Begin test here"""
    filename = sys.argv[1]
    queue = Queue()
    thread = threading.Thread(target=file_reader, args=(filename, queue))
    thread.daemon = True
    thread.start()

    client = Client("127.0.0.1", 10000, queue)

    while 1:
        pass


if __name__ == "__main__":
    main()
