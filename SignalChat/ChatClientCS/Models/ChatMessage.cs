using System;
using System.Linq;

namespace ChatClientCS.Models
{
    public class ChatMessage
    {
        public string Message { get; set; }
        public string Author { get; set; }
        public DateTime Time { get; set; }
        public string Picture { get; set; }
        public bool IsOriginNative { get; set; }

        private const string AuthorMagic = "&Author&";
        private const string DataMagic = "&DATA&";

        public ChatMessage() { }

        public ChatMessage(string data, string selectedParticipant)
        {

            int startAuthorIndex = data.IndexOf(AuthorMagic, StringComparison.Ordinal);
            int startDataIndex = data.IndexOf(DataMagic, StringComparison.Ordinal);
            if (startAuthorIndex < 0 || startDataIndex < 0)
            {
                throw new Exception();
            }

            string author = data.Substring(startAuthorIndex + AuthorMagic.Length, startDataIndex - startAuthorIndex - AuthorMagic.Length);
            string messageData = data.Substring(startDataIndex + DataMagic.Length, data.Length - startDataIndex - DataMagic.Length);
            Console.WriteLine($@"Author={author}, Data={messageData}");
            if (string.IsNullOrEmpty(author) || string.IsNullOrEmpty(messageData))
            {
                throw new Exception();
            }

            Author = author;
            Message = messageData;
            IsOriginNative = !selectedParticipant.Equals(author);
            Time = DateTime.Now;
        }
    }
}
