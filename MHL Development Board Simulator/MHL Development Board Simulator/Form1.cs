using Interface_Library;
using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Threading;
using System.Windows.Forms;


namespace MHL_Development_Board_Simulator
{
    public partial class Form1 : Form
    {
        
        String selectedPortName = "COM5";

        int[] button_output_state = new int[16];
        int[] button_input_state  = new int[16];

        static bool _continue;

        Serial serialInterface = new Serial();

        delegate void UpdateInputsCallback();

        public Form1()
        {
            _continue = true;

            serialInterface.DataReceived += DataReceivedEventHandler;

            InitializeComponent();
            InitializeIOStates();
            InitializeComPortList();

        }

        /// <summary>
        /// Initialize the I/O states to their
        /// default state (LOW)
        /// </summary>
        private void InitializeIOStates()
        {
            for(int i=0;i<16;i++)
            {
                button_output_state[i] = 0;
                button_input_state[i] = 0;
            }
        }

        /// <summary>
        /// Retrieve the list of the available
        /// COM ports
        /// </summary>
        private void InitializeComPortList()

        {
            foreach (string str in serialInterface.GetAllPorts())
            {
                selectedPort.Items.Add(str);
            }
        }

        private void connect_button_Click(object sender, EventArgs e)
        {

            try
            {
                serialInterface.OpenSerialPort(selectedPortName, 115200);
                serialInterface.isConnected = 1;
                establish_connection_sequence();  // Send Command to the MCU to light the PC connection LED
                timer1.Start();
            }
            catch(Exception ex1)
            {
                MessageBox.Show(ex1.Message);
                serialInterface.isConnected = 0;
            }

            validate_con_dis_buttons();

            
            //byte[] bArray = new byte[] { 0x61 , 0x62 , 0x63 , 0x64 };
            //bArray = Encoding.ASCII.GetBytes("a");

            /*String result;
            if((result = Serial.WriteToSerial(bArray))!="Success")
            {
                MessageBox.Show(result);
            }*/

        }

        /// <summary>
        /// Retrieve the selected COM Port to be opened
        /// after choosing on the drop-down list
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void selectedPort_SelectedIndexChanged(object sender, EventArgs e)
        {
            selectedPortName = (String) selectedPort.SelectedItem;
            //MessageBox.Show(selectedPortName);
        }

        /// <summary>
        /// Closes the serial port after pressing
        /// the disconnect button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void disconnect_button_Click(object sender, EventArgs e)
        {
            try
            {
                //Send disconnect command to board
                terminate_connection_sequence();

                //Close the port
                serialInterface.CloseSerialPort();
                serialInterface.isConnected = 0;

                timer1.Stop();
            }
            catch (Exception ex2)
            { 
                MessageBox.Show(ex2.Message);
            }

            validate_con_dis_buttons();
        }

        /// <summary>
        /// Update the text of the "Connect" button based on
        /// the status of the connection
        /// </summary>
        private void validate_con_dis_buttons()
        {

            if (serialInterface.isConnected == 1)
            {
                connect_button.Enabled = false;
                connect_button.Text = "Connected!";
            }
            else
            {
                connect_button.Enabled = true;
                connect_button.Text = "Connect";
            }

        }

        /// <summary>
        /// Transmit a specific sequence of bytes to 
        /// establish connection with the board
        /// </summary>
        private void establish_connection_sequence()
        {
            byte[] tmp_buffer = { 0xCB, 0xA5, 0x75, 0x31 };
            serialInterface.WriteToSerial(tmp_buffer);
        }
        
        private void terminate_connection_sequence()
        {
            byte[] tmp_buffer = { 0x00, 0xA7, 0x00, 0x00 };
            serialInterface.WriteToSerial(tmp_buffer);
        }

        private void validate_output_states()
        {
            byte[] tmp_buffer = { 0x00, 0xA2, 0x00, 0x00 };
            serialInterface.WriteToSerial(tmp_buffer);
        }

        /// <summary>
        /// Send command to the board to update all the
        /// outputs
        /// </summary>
        private void transmit_output_states()
        {
            byte[] tmp_buffer = { 0x00, 0xA1, 0x00, 0x00 };

            byte tmp_low_byte = new Byte();
            tmp_low_byte = 0x00;
            for(int i=0;i<8;i++)
            {
                if(button_output_state[i]==1)
                {
                    tmp_low_byte |= (byte)(1 << i);
                }
            }

            tmp_buffer[3] = tmp_low_byte;

            byte tmp_high_byte = new Byte();
            tmp_high_byte = 0x00;
            for (int i = 0; i < 8; i++)
            {
                if (button_output_state[i+8] == 1)
                {
                    tmp_high_byte |= (byte)(1 << i);
                }
            }

            tmp_buffer[2] = tmp_high_byte;

            serialInterface.WriteToSerial(tmp_buffer);
        }


        public void DataReceivedEventHandler(object source, EventArgs e)
        {
            byte[] tmp_data = new byte[2];

            tmp_data[0] = serialInterface.receive_buffer[2];
            tmp_data[1] = serialInterface.receive_buffer[1];
            

            if(serialInterface.receive_buffer[0] == 0xB1)
            {
                BitArray inputs_bit_array = new BitArray(tmp_data);

                //MessageBox.Show(inputs_bit_array.ToString());
                
                for(int i=0;i<16;i++)
                {
                    if(inputs_bit_array[i] == true)
                    {
                        button_input_state[i] = 1;
                    }
                    else if(inputs_bit_array[i] == false)
                    {
                        button_input_state[i] = 0;
                    }
                }

                update_input_buttons_text();
            }

            else if(serialInterface.receive_buffer[0] == 0xB2)
            {
                BitArray outputs_bit_array = new BitArray(tmp_data);
                                
                for (int i = 0; i < 16; i++)
                {
                    if (outputs_bit_array[i] == true)
                    {
                        button_output_state[i] = 1;
                    }
                    else if (outputs_bit_array[i] == false)
                    {
                        button_output_state[i] = 0;
                    }
                }


            }
            
        }


        /*public static void ReadSerial()
        {
            
            while (_continue)
            {
                try
                {
                    byte[] readBuffer;
                    
                    string message = Encoding.ASCII.GetString(readBuffer);
                    MessageBox.Show(message);
                }
                catch (TimeoutException) { }
            }
        }*/

        /// <summary>
        /// Read data from Serial Port
        /// </summary>
        /*public void Read()
        {
            while (_continue)
            {
                try
                {
                    string message = Serial.
                    Console.WriteLine(message);
                }
                catch (TimeoutException) { }
            }
        }*/


        /*---------------------- Output Buttons ------------------------*/

        private void button_output_1_Click(object sender, EventArgs e)
        {
            if(button_output_state[0] == 0)
            {
                button_output_state[0] = 1;
            }
            else if(button_output_state[0] == 1)
            {
                button_output_state[0] = 0;
            }

            transmit_output_states();
            update_output_buttons_text();
        }

        private void button_output_2_Click(object sender, EventArgs e)
        {
            if (button_output_state[1] == 0)
            {
                button_output_state[1] = 1;
            }
            else if (button_output_state[1] == 1)
            {
                button_output_state[1] = 0;
            }

            transmit_output_states();
            update_output_buttons_text();
        }

        private void button_output_3_Click(object sender, EventArgs e)
        {
            if (button_output_state[2] == 0)
            {
                button_output_state[2] = 1;
            }
            else if (button_output_state[2] == 1)
            {
                button_output_state[2] = 0;
            }

            transmit_output_states();
            update_output_buttons_text();
        }

        private void button_output_4_Click(object sender, EventArgs e)
        {
            if (button_output_state[3] == 0)
            {
                button_output_state[3] = 1;
            }
            else if (button_output_state[3] == 1)
            {
                button_output_state[3] = 0;
            }

            transmit_output_states();
            update_output_buttons_text();
        }

        private void button_output_5_Click(object sender, EventArgs e)
        {
            if (button_output_state[4] == 0)
            {
                button_output_state[4] = 1;
            }
            else if (button_output_state[4] == 1)
            {
                button_output_state[4] = 0;
            }

            transmit_output_states();
            update_output_buttons_text();
        }

        private void button_output_6_Click(object sender, EventArgs e)
        {
            if (button_output_state[5] == 0)
            {
                button_output_state[5] = 1;
            }
            else if (button_output_state[5] == 1)
            {
                button_output_state[5] = 0;
            }

            transmit_output_states();
            update_output_buttons_text();
        }

        private void button_output_7_Click(object sender, EventArgs e)
        {
            if (button_output_state[6] == 0)
            {
                button_output_state[6] = 1;
            }
            else if (button_output_state[6] == 1)
            {
                button_output_state[6] = 0;
            }

            transmit_output_states();
            update_output_buttons_text();
        }

        private void button_output_8_Click(object sender, EventArgs e)
        {
            if (button_output_state[7] == 0)
            {
                button_output_state[7] = 1;
            }
            else if (button_output_state[7] == 1)
            {
                button_output_state[7] = 0;
            }

            transmit_output_states();
            update_output_buttons_text();
        }

        private void button_output_9_Click(object sender, EventArgs e)
        {
            if (button_output_state[8] == 0)
            {
                button_output_state[8] = 1;
            }
            else if (button_output_state[8] == 1)
            {
                button_output_state[8] = 0;
            }

            transmit_output_states();
            update_output_buttons_text();
        }

        private void button_output_10_Click(object sender, EventArgs e)
        {
            if (button_output_state[9] == 0)
            {
                button_output_state[9] = 1;
            }
            else if (button_output_state[9] == 1)
            {
                button_output_state[9] = 0;
            }

            transmit_output_states();
            update_output_buttons_text();
        }

        private void button_output_11_Click(object sender, EventArgs e)
        {
            if (button_output_state[10] == 0)
            {
                button_output_state[10] = 1;
            }
            else if (button_output_state[10] == 1)
            {
                button_output_state[10] = 0;
            }

            transmit_output_states();
            update_output_buttons_text();
        }

        private void button_output_12_Click(object sender, EventArgs e)
        {
            if (button_output_state[11] == 0)
            {
                button_output_state[11] = 1;
            }
            else if (button_output_state[11] == 1)
            {
                button_output_state[11] = 0;
            }

            transmit_output_states();
            update_output_buttons_text();
        }

        private void button_output_13_Click(object sender, EventArgs e)
        {
            if (button_output_state[12] == 0)
            {
                button_output_state[12] = 1;
            }
            else if (button_output_state[12] == 1)
            {
                button_output_state[12] = 0;
            }

            transmit_output_states();
            update_output_buttons_text();
        }

        private void button_output_14_Click(object sender, EventArgs e)
        {
            if (button_output_state[13] == 0)
            {
                button_output_state[13] = 1;
            }
            else if (button_output_state[13] == 1)
            {
                button_output_state[13] = 0;
            }

            transmit_output_states();
            update_output_buttons_text();
        }

        private void button_output_15_Click(object sender, EventArgs e)
        {
            if (button_output_state[14] == 0)
            {
                button_output_state[14] = 1;
            }
            else if (button_output_state[14] == 1)
            {
                button_output_state[14] = 0;
            }

            transmit_output_states();
            update_output_buttons_text();
        }

        private void button_output_16_Click(object sender, EventArgs e)
        {
            if (button_output_state[15] == 0)
            {
                button_output_state[15] = 1;
            }
            else if (button_output_state[15] == 1)
            {
                button_output_state[15] = 0;
            }

            transmit_output_states();
            update_output_buttons_text();
            
            //validate_output_states();

        }

        private void update_output_buttons_text()
        {

                button_output_1.Text = button_output_state[0].ToString();
                button_output_2.Text = button_output_state[1].ToString();
                button_output_3.Text = button_output_state[2].ToString();
                button_output_4.Text = button_output_state[3].ToString();
                button_output_5.Text = button_output_state[4].ToString();
                button_output_6.Text = button_output_state[5].ToString();
                button_output_7.Text = button_output_state[6].ToString();
                button_output_8.Text = button_output_state[7].ToString();
                button_output_9.Text = button_output_state[8].ToString();
                button_output_10.Text = button_output_state[9].ToString();
                button_output_11.Text = button_output_state[10].ToString();
                button_output_12.Text = button_output_state[11].ToString();
                button_output_13.Text = button_output_state[12].ToString();
                button_output_14.Text = button_output_state[13].ToString();
                button_output_15.Text = button_output_state[14].ToString();
                button_output_16.Text = button_output_state[15].ToString();
            
        }

        private void update_input_buttons_text()
        {
            if(this.button_input_1.InvokeRequired)
            {
                UpdateInputsCallback d = new UpdateInputsCallback(update_input_buttons_text);
                try
                {
                    this.Invoke(d);
                }
                catch(Exception t_ex)
                {
                    MessageBox.Show(t_ex.Message);
                }
            }
            else
            {
                button_input_1.Text = button_input_state[0].ToString();
                button_input_2.Text = button_input_state[1].ToString();
                button_input_3.Text = button_input_state[2].ToString();
                button_input_4.Text = button_input_state[3].ToString();
                button_input_5.Text = button_input_state[4].ToString();
                button_input_6.Text = button_input_state[5].ToString();
                button_input_7.Text = button_input_state[6].ToString();
                button_input_8.Text = button_input_state[7].ToString();
                button_input_9.Text = button_input_state[8].ToString();
                button_input_10.Text = button_input_state[9].ToString();
                button_input_11.Text = button_input_state[10].ToString();
                button_input_12.Text = button_input_state[11].ToString();
                button_input_13.Text = button_input_state[12].ToString();
                button_input_14.Text = button_input_state[13].ToString();
                button_input_15.Text = button_input_state[14].ToString();
                button_input_16.Text = button_input_state[15].ToString();
            }

            update_input_buttons_color();

        }

        private void update_input_buttons_color()
        {
            if(button_input_state[0]==1)
                button_input_1.BackColor = Color.Green;
            else
                button_input_1.BackColor = Color.LightGray;
            
            if (button_input_state[1] == 1)
                button_input_2.BackColor = Color.Green;
            else
                button_input_2.BackColor = Color.LightGray;

            if (button_input_state[2] == 1)
                button_input_3.BackColor = Color.Green;
            else
                button_input_3.BackColor = Color.LightGray;

            if (button_input_state[3] == 1)
                button_input_4.BackColor = Color.Green;
            else
                button_input_4.BackColor = Color.LightGray;

            if (button_input_state[4] == 1)
                button_input_5.BackColor = Color.Green;
            else
                button_input_5.BackColor = Color.LightGray;

            if (button_input_state[5] == 1)
                button_input_6.BackColor = Color.Green;
            else
                button_input_6.BackColor = Color.LightGray;

            if (button_input_state[6] == 1)
                button_input_7.BackColor = Color.Green;
            else
                button_input_7.BackColor = Color.LightGray;

            if (button_input_state[7] == 1)
                button_input_8.BackColor = Color.Green;
            else
                button_input_8.BackColor = Color.LightGray;

            if (button_input_state[8] == 1)
                button_input_9.BackColor = Color.Green;
            else
                button_input_9.BackColor = Color.LightGray;

            if (button_input_state[9] == 1)
                button_input_10.BackColor = Color.Green;
            else
                button_input_10.BackColor = Color.LightGray;

            if (button_input_state[10] == 1)
                button_input_11.BackColor = Color.Green;
            else
                button_input_11.BackColor = Color.LightGray;

            if (button_input_state[11] == 1)
                button_input_12.BackColor = Color.Green;
            else
                button_input_12.BackColor = Color.LightGray;

            if (button_input_state[12] == 1)
                button_input_13.BackColor = Color.Green;
            else
                button_input_13.BackColor = Color.LightGray;

            if (button_input_state[13] == 1)
                button_input_14.BackColor = Color.Green;
            else
                button_input_14.BackColor = Color.LightGray;

            if (button_input_state[14] == 1)
                button_input_15.BackColor = Color.Green;
            else
                button_input_15.BackColor = Color.LightGray;

            if (button_input_state[15] == 1)
                button_input_16.BackColor = Color.Green;
            else
                button_input_16.BackColor = Color.LightGray;

        }

        
        /*--------------------- Inputs Buttons -----------------------*/

        private void button_input_1_Click(object sender, EventArgs e)
        {

        }

        private void button_input_2_Click(object sender, EventArgs e)
        {

        }

        private void button_input_3_Click(object sender, EventArgs e)
        {

        }

        private void button_input_4_Click(object sender, EventArgs e)
        {

        }

        private void button_input_5_Click(object sender, EventArgs e)
        {

        }

        private void button_input_6_Click(object sender, EventArgs e)
        {

        }

        private void button_input_7_Click(object sender, EventArgs e)
        {

        }

        private void button_input_8_Click(object sender, EventArgs e)
        {

        }

        private void button_input_9_Click(object sender, EventArgs e)
        {

        }

        private void button_input_10_Click(object sender, EventArgs e)
        {

        }

        private void button_input_11_Click(object sender, EventArgs e)
        {

        }

        private void button_input_12_Click(object sender, EventArgs e)
        {

        }

        private void button_input_13_Click(object sender, EventArgs e)
        {

        }

        private void button_input_14_Click(object sender, EventArgs e)
        {

        }

        private void button_input_15_Click(object sender, EventArgs e)
        {

        }

        private void button_input_16_Click(object sender, EventArgs e)
        {

        }


        private void button1_Click(object sender, EventArgs e)
        {
            
            byte[] receive_buffer = { };
            
            //receive_buffer = serialInterface.ReadFromSerial();

            MessageBox.Show(Encoding.ASCII.GetString(receive_buffer));
        }

        /// <summary>
        /// Create a 10msec clock pulse
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void clk_button_Click(object sender, EventArgs e)
        {
            byte[] tmp_buffer = { 0x00, 0xA4, 0x00, 0x00 };
            serialInterface.WriteToSerial(tmp_buffer);
        }

        /// <summary>
        /// Timer that sends probe signal every 1sec to
        /// notify the board that the connection is still
        /// alive
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void timer1_Tick(object sender, EventArgs e)
        {
            if(serialInterface.isConnected == 1)
            {
                byte[] tmp_buffer = { 0x00, 0xA6, 0x00, 0x00 };

                serialInterface.WriteToSerial(tmp_buffer);
            }
        }

        /// <summary>
        /// Refresh COM Port list (for new available
        /// ports)
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void refresh_button_Click(object sender, EventArgs e)
        {
            selectedPort.Items.Clear();

            foreach (string str in serialInterface.GetAllPorts())
            {
                selectedPort.Items.Add(str);
            }
        }
    }
}
