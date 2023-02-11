using System;
using System.IO;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Forms;

namespace ChatClientCS.Services
{
    class Client
    {
        private static readonly Object lock_obj = new Object();

        private TcpClient _tcpClient;
        byte[] bytes = new byte[65536];

        public string Ip{ get; set; }

        public int Port { get; private set; }

        public Client()
        {
            SetConnection();
        }

        private void SetConnection()
        {
            _tcpClient = new TcpClient();
            var sr = new StreamReader("config.txt");
            Ip = sr.ReadLine()?.Split('=')[1];
            Port = Convert.ToInt32(sr.ReadLine()?.Split('=')[1]);
        }

        public async Task Start()
        {
            await _tcpClient.ConnectAsync(Ip, Port);
        }

        public void Close()
        {
            _tcpClient.Close();
        }

        public async Task<string> ReceivePartFromServer(int sizeLen)
        {
            if (sizeLen == 0)
                return "";
            byte[] ans = new byte[sizeLen];
            var stream = _tcpClient.GetStream();
            var bytesRead = await stream.ReadAsync(ans, 0, sizeLen);
            return Encoding.ASCII.GetString(ans);
        }

        public async Task<string[]> ReceiveMsgFromServer()
        {
            string[] res = new string[6];

            try
            { 
                int code = int.Parse(await ReceivePartFromServer(3));
                if (code == 101)
                {
                    res[0] = await ReceivePartFromServer(5); // size of the file
                    res[1] = await ReceivePartFromServer(int.Parse(res[0])); // file content
                    res[2] = await ReceivePartFromServer(2); // curr username size
                    res[3] = await ReceivePartFromServer(int.Parse(res[2])); // curr username
                    res[4] = await ReceivePartFromServer(5); // size of all usernames
                    res[5] = await ReceivePartFromServer(int.Parse(res[4])); // all usernames

                }
            }
            catch
            {
                // ignored
            }

            return res;
        }

        public async Task SendUpdateToServer(string secondUser, string msg)
        {
            const int code = 204;
            string s = Convert.ToString(code).PadLeft(3, '0') +
                       secondUser.Length.ToString().PadLeft(2, '0')  + secondUser +
                       msg.Length.ToString().PadLeft(5, '0') + msg;
            var msgToSend = Encoding.ASCII.GetBytes(Convert.ToString(s));

            var stream = _tcpClient.GetStream();
            await stream.WriteAsync(msgToSend, 0, msgToSend.Length);
        }

        public async Task<string[]> SendAndReceiveLogin(string username)
        {
            string s = "200" + username.Length.ToString().PadLeft(2, '0') + username;
            byte[] msg = Encoding.ASCII.GetBytes(Convert.ToString(s));

            var stream = _tcpClient.GetStream();
            await stream.WriteAsync(msg, 0, msg.Length);

            return await ReceiveMsgFromServer();
        }
    }

}
