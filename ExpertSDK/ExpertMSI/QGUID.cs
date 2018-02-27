using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace QGUID
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine(Guid.NewGuid().ToString());
        }
    }
}
