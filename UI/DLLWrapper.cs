using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
//using MonoMac.ObjCRuntime;

namespace UI
{
    public class DLLWrapper
    {
		[DllImport ("libSystem.dylib")]
		public static extern IntPtr dlopen (string path, int mode);

		[DllImport ("libSystem.dylib")]
		public static extern int dlclose (IntPtr handle);

		[DllImport ("libSystem.dylib")]
		public static extern IntPtr dlsym (IntPtr handle, string symbol);

		[DllImport ("libSystem.dylib")]
		public static extern string dlerror ();

		///<summary>
        ///载入动态链接库形式的策略文件，对mac的dlopen函数进行封装
        ///</summary>
        ///<param name="dllFilePath">要载入的动态链接库文件</param>
		public static IntPtr LoadLibrary(String dllFilePath)
		{
			return dlopen (dllFilePath, 1);	/*0 - RTLD_LAZY, 1 - RTLD_NOW*/
		}

		///<summary>
        ///释放动态链接库
        ///</summary>
        ///<param name="handle">已载入的动态链接库</param>
		public static int FreeLibrary(IntPtr handle)
		{
			return dlclose (handle);
		}

        ///<summary>
        ///通过非托管函数名转换为对应的委托
        ///</summary>
        ///<param name="dllModule">通过LoadLibrary获得的DLL句柄</param>
        ///<param name="functionName">非托管函数名</param>
        ///<param name="t">对应的委托类型</param>
        ///<returns>委托实例，可强制转换为适当的委托类型</returns>
        public static Delegate GetFunctionAddress(IntPtr dllModule, string functionName, Type t)
        {
            IntPtr address = dlsym (dllModule, functionName);
			if (address == IntPtr.Zero)
                return null;
            else
                return Marshal.GetDelegateForFunctionPointer(address, t);
        }
    }
}