"""
Prototype of reconstructing a 2d scan of a phantom.
"""

__author__ = "Saveliy Yusufov"
__email__ = "sy2685@columbia.edu"
__version__ = "1.0"

import sys
import logging
import threading
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import scipy.signal as sig


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


class DataHandler:
    """
    This class is meant to simulate the microcontoller that will connect and
    send data to the device on which the Ultrasound image is reconstructed.
    """

    def __init__(self, data_queue):
        self.data_queue = data_queue

        logger.info("data sender thread started!")
        thread = threading.Thread(target=self.data_sender, args=())
        thread.daemon = True
        thread.start()

    def data_sender(self):
        logger.info("data_sender thread started")

        while True:
            value = self.data_queue.get()
            print(value)


def main():
    """Begin image reconstruction here"""
    filename = sys.argv[1]

    data = pd.read_csv(filename, header=None, sep=' ')
    x = data.iloc[:, 0]
    y = data.iloc[:, 4]

    max_tx_val = 150
    peaks = sig.find_peaks(y, height=max_tx_val)
    peak_indices = peaks[0]

    start = peak_indices[0]
    end = peak_indices[1]

    jet_cmap = plt.get_cmap('jet')
    colors = jet_cmap(y[start:end])

    for i, color in enumerate(colors):
        plt.scatter(start+i, 300, color=color, marker='s')


    plt.plot(x[start:end], y[start:end])
    plt.show()


if __name__ == "__main__":
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

    main()
