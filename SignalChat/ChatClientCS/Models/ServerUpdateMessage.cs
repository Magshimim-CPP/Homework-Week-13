using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ChatClientCS.Models
{
    public class ServerUpdateMessage
    {
        public List<User> Users { get; set; }

        public string ParticipantName { get; set; }

        public string ChatData { get; set; }
    }
}
