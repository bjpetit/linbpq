/*
 * Created by SharpDevelop.
 * User: johnw_000
 * Date: 20/03/2015
 * Time: 14:32
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using System.Net.Sockets;


namespace TermTCP
{
	/// <summary>
	/// Description of MainForm.
	/// </summary>

	public partial class MainForm : Form
	{
		TcpClient tcpClientC;
        NetworkStream netStream;
        byte[] myReadBuffer = new byte[1024];

		public MainForm()
		{
			//
			// The InitializeComponent() call is required for Windows Forms designer support.
			//
			InitializeComponent();
			
			//
			// TODO: Add constructor code after the InitializeComponent() call.
			//
		}
		void TextBox1TextChanged(object sender, EventArgs e)
		{
	
		}
		void ConnectToolStripMenuItemClick(object sender, EventArgs e)
		{
			string caption = "TermTCP";
			MessageBoxButtons buttons = MessageBoxButtons.OK;
			DialogResult result;

			try
			{
				tcpClientC = new TcpClient ("127.0.0.1", 8011);
				
				netStream = tcpClientC.GetStream();
				netStream.BeginRead(myReadBuffer, 0, myReadBuffer.Length,
				      new AsyncCallback(myReadCallBack), netStream);
			}
			catch (Exception ex)
			{
				 MessageBox.Show(ex.Message, caption, buttons);
			}
		}
		
		public void myReadCallBack(IAsyncResult ar)
        {
            NetworkStream myNetworkStream = (NetworkStream)ar.AsyncState;
            int count= 0;

            try
            {
                count = myNetworkStream.EndRead(ar);

 
                myNetworkStream.BeginRead(myReadBuffer, 0, myReadBuffer.Length,
                                                       new AsyncCallback(myReadCallBack),
                                                       myNetworkStream);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }
	}
}
