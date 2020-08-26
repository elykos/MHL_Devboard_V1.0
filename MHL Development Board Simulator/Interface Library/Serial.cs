using System;
using System.Collections.Generic;
using System.Text;
using System.IO.Ports;

namespace Interface_Library
{
    public class Serial
    {
        
        public SerialPort serialPort;
        public int isConnected;

        public byte[] receive_buffer = new Byte[3];

        public delegate void DataReceivedEventHandler(object source, EventArgs e);

        public event DataReceivedEventHandler DataReceived;

        /// <summary>
        /// List that contains the available ports
        /// </summary>
        public List<string> portsList;

        /// <summary>
        /// Lists all the available serial ports
        /// </summary>
        /// <returns>
        /// Returns a list (of strings) of the available
        /// serial ports
        /// </returns>
        public List<string> GetAllPorts()
        {
            portsList = new List<string>();

            foreach (string portName in SerialPort.GetPortNames())
            {
                portsList.Add(portName);
            }
            return portsList;
        }

        /// <summary>
        /// Opens specified serial port
        /// </summary>
        /// <param name="Port"></param>
        /// String name of serial port
        /// <param name="bRate"></param>
        /// Baud Rate
        /// <returns></returns>
        public int OpenSerialPort(string Port,int bRate)
        {

            serialPort = new SerialPort(Port, bRate, Parity.None , 8 , StopBits.One);
            serialPort.Handshake = Handshake.None;
            serialPort.ReceivedBytesThreshold = 3;
            serialPort.DataReceived += new SerialDataReceivedEventHandler(myDataReceivedHandler);
            if(serialPort.IsOpen == false)
            {
                serialPort.Open();
            }

            return 0;
        }

        /// <summary>
        /// Close Serial port
        /// </summary>
        /// <param name="Port"></param>
        /// <returns></returns>
        public int CloseSerialPort()
        {
            serialPort.Close();
            return 0;
        }
        
        /// <summary>
        /// Send the data buffer to serial port
        /// </summary>
        /// <param name="sendBuffer"></param>
        /// The buffer to be sent (4 bytes)
        /// <returns></returns>
        public String WriteToSerial(byte[] sendBuffer)
        {
            
            try
            {
                serialPort.Write(sendBuffer,0,4);
            }
            catch(Exception ex3)
            {
                return ex3.Message;
            }

            return "Success";
        }


        protected virtual void OnDataReceived()
        {
            if(DataReceived != null)
            {
                DataReceived(this, EventArgs.Empty);
            }
        }


        /*public  byte[] ReadFromSerial()
        {
            byte[] readBuffer = { 0x00 , 0x00 , 0x00 };

            try
            {
                serialPort.Read(readBuffer, 0, 3);
            }

            catch(Exception ex4)
            {
                
            }

            return readBuffer;
        }*/


        public void myDataReceivedHandler(object sender,
                        SerialDataReceivedEventArgs e)
        {
            serialPort.Read(receive_buffer, 0, 3);
            OnDataReceived();
        }
    }
}
