using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using ChatClientCS.Enums;
using ChatClientCS.Models;
using System.Net;
using Microsoft.AspNet.SignalR.Client;

namespace ChatClientCS.Services
{
    public class ChatService : IChatService
    {
        public event Action<string, string, MessageType> NewTextMessage;
        public event Action<string, byte[], MessageType> NewImageMessage;
        public event Action<string> ParticipantDisconnected;
        public event Action<User> ParticipantLoggedIn;
        public event Action<string> ParticipantLoggedOut;
        public event Action<string> ParticipantReconnected;
        public event Action ConnectionReconnecting;
        public event Action ConnectionReconnected;
        public event Action ConnectionClosed;
        public event Action<string> ParticipantTyping;

        private IHubProxy hubProxy;
        private HubConnection connection;
        private string url = "http://localhost:8080/signalchat";
        private Client client;

        public async Task ConnectAsync()
        {
            connection = new HubConnection(url);
            hubProxy = connection.CreateHubProxy("ChatHub");
            hubProxy.On<User>("ParticipantLogin", (u) => ParticipantLoggedIn?.Invoke(u));
            hubProxy.On<string>("ParticipantLogout", (n) => ParticipantLoggedOut?.Invoke(n));
            hubProxy.On<string>("ParticipantDisconnection", (n) => ParticipantDisconnected?.Invoke(n));
            hubProxy.On<string>("ParticipantReconnection", (n) => ParticipantReconnected?.Invoke(n));
            hubProxy.On<string, string>("BroadcastTextMessage", (n, m) => NewTextMessage?.Invoke(n, m, MessageType.Broadcast));
            hubProxy.On<string, byte[]>("BroadcastPictureMessage", (n, m) => NewImageMessage?.Invoke(n, m, MessageType.Broadcast));
            hubProxy.On<string, string>("UnicastTextMessage", (n, m) => NewTextMessage?.Invoke(n, m, MessageType.Unicast));
            hubProxy.On<string, byte[]>("UnicastPictureMessage", (n, m) => NewImageMessage?.Invoke(n, m, MessageType.Unicast));
            hubProxy.On<string>("ParticipantTyping", (p) => ParticipantTyping?.Invoke(p));

            connection.Reconnecting += Reconnecting;
            connection.Reconnected += Reconnected;
            connection.Closed += Disconnected;
            
            ServicePointManager.DefaultConnectionLimit = 10;
            client = new Client();
            await client.Start();
            //await connection.Start();
        }

        private void Disconnected()
        {
            ConnectionClosed?.Invoke();
        }

        private void Reconnected()
        {
            ConnectionReconnected?.Invoke();
        }

        private void Reconnecting()
        {
            ConnectionReconnecting?.Invoke();
        }

        public async Task<List<User>> LoginAsync(string name, byte[] photo)
        {
            string[] serverUpdateMessage = await client.SendAndReceiveLogin(name);
            string users = serverUpdateMessage[5];
            return getUsers(name, users);
        }

        private List<User> getUsers(string name, string allUsers)
        {
            string[] spllitedUsers = allUsers.Split('&');
            return spllitedUsers.Select(us => new User { Name = us }).Where(us => !us.Name.Equals(name)).ToList();
        }

        public async Task LogoutAsync()
        {
            await hubProxy.Invoke("Logout");
        }

        public async Task SendBroadcastMessageAsync(string msg)
        {
            await hubProxy.Invoke("BroadcastTextMessage", msg);
        }

        public async Task SendBroadcastMessageAsync(byte[] img)
        {
            await hubProxy.Invoke("BroadcastImageMessage", img);
        }

        public async Task SendUnicastMessageAsync(string recepient, string msg)
        {
            await client.SendUpdateToServer(recepient, msg);
            //await hubProxy.Invoke("UnicastTextMessage", new object[] { recepient, msg });
        }

        public async Task SendUnicastMessageAsync(string recepient, byte[] img)
        {
            await hubProxy.Invoke("UnicastImageMessage", new object[] { recepient, img });
        }

        public async Task TypingAsync(string recepient)
        {
            await hubProxy.Invoke("Typing", recepient);
        }

        public async Task<ServerUpdateMessage> GetServerUpdates(string currentUser, string recipient)
        {
            await client.SendUpdateToServer(recipient ?? string.Empty , string.Empty);
            string[] serverUpdateMessage = await client.ReceiveMsgFromServer();
            string users = serverUpdateMessage[5];
            var sum = new ServerUpdateMessage {Users = getUsers(currentUser, users), ParticipantName = serverUpdateMessage[3], ChatData = serverUpdateMessage[1]};
            return sum;
        }
    }
}