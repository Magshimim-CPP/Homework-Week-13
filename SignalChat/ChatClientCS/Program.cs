using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace ChatClientCS
{
    public class Program
    {
        [STAThreadAttribute]
        public static void Main()
        {
            var assemblies = new Dictionary<string, Assembly>();
            var executingAssembly = Assembly.GetExecutingAssembly();
            var resources = executingAssembly.GetManifestResourceNames().Where(n => n.EndsWith(".dll"));

            foreach (string resource in resources)
            {
                using (var stream = executingAssembly.GetManifestResourceStream(resource))
                {
                    if (stream == null)
                        continue;

                    var bytes = new byte[stream.Length];
                    stream.Read(bytes, 0, bytes.Length);
                    try
                    {
                        assemblies.Add(resource, Assembly.Load(bytes));
                    }
                    catch (Exception ex)
                    {
                        System.Diagnostics.Debug.Print($"Failed to load: {resource}, Exception: {ex.Message}");
                    }
                }
            }

            AppDomain.CurrentDomain.AssemblyResolve += (s, e) =>
            {
                var assemblyName = new AssemblyName(e.Name);
                var path = $"{assemblyName.Name}.dll";
                return assemblies.ContainsKey(path) ? assemblies[path] : null;
            };

            App.Main();
        }
    }
}

