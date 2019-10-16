#pragma warning disable 1591

using System;
using System.Runtime.InteropServices;
using System.ComponentModel;

using static eldbus.EldbusMessageNativeFunctions;

namespace eldbus
{
/// <summary>
/// Representation for Timeout flags.
/// <para>Since EFL 1.23.</para>
/// </summary>
public static class Timeout
{
    /// <summary>
    /// Infinite flag.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public const int Infinite = 0x7fffffff;
}

/// <summary>
/// The path to an object.
/// <para>Since EFL 1.23.</para>
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct ObjectPath
{
    /// <summary>
    /// The string of the path.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public string value;

    /// <summary>
    /// Constructor
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="str">The string of the path.</param>
    public ObjectPath(string str)
    {
        value = str;
    }

    /// <summary>
    /// Conversion operator of ObjectPath from string.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="str">The string of the path.</param>
    public static implicit operator ObjectPath(string str)
    {
        return new ObjectPath(str);
    }

    /// <summary>
    /// Conversion operator of string from ObjectPath.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="path">The ObjectPath to be converted.</param>
    public static implicit operator string(ObjectPath path)
    {
        return path.value;
    }
}

/// <summary>
/// String to a signature.
/// <para>Since EFL 1.23.</para>
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct SignatureString
{
    /// <summary>
    /// The string of the signature.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public string value;

    /// <summary>
    /// Constructor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="str">The string of the signature.</param>
    public SignatureString(string str)
    {
        value = str;
    }

    /// <summary>
    /// Conversion operator of SignatureString from string.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="str">The string of the signature.</param>
    public static implicit operator SignatureString(string str)
    {
        return new SignatureString(str);
    }

    /// <summary>
    /// Conversion operator of string from SignatureString.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="sig">The SignatureString to be conversion.</param>
    public static implicit operator string(SignatureString sig)
    {
        return sig.value;
    }
}

/// <summary>
/// Representation for Unix file descriptor.
/// <para>Since EFL 1.23.</para>
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct UnixFd
{
    /// <summary>
    /// The value of the file descriptor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public Int32 value;

    /// <summary>
    /// Constructor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="fd">The file descriptor.</param>
    public UnixFd(Int32 fd)
    {
        value = fd;
    }

    /// <summary>
    /// Conversion operator of UnixFd from Int32.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="fd">The file descriptor.</param>
    public static implicit operator UnixFd(Int32 fd)
    {
        return new UnixFd(fd);
    }

    /// <summary>
    /// Conversion operator of Int32 from UnixFd.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="unix_fd">The UnixFd to be converted.</param>
    public static implicit operator Int32(UnixFd unix_fd)
    {
        return unix_fd.value;
    }
}

/// <summary>
/// Arguments of EldBus.
/// <para>Since EFL 1.23.</para>
/// </summary>
public static class Argument
{
    /// <summary>
    /// The type of a byte.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public class ByteType
    {
        /// <summary>
        /// The code of the byte.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const char Code = 'y';
        /// <summary>
        /// The signature of the byte.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const string Signature = "y";
    }

    /// <summary>
    /// The type of a boolean
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public class BooleanType
    {
        /// <summary>
        /// The code of the boolean.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const char Code = 'b';
        /// <summary>
        /// The signature of the boolean.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const string Signature = "b";
    }

    /// <summary>
    /// The type of a Int16.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public class Int16Type
    {
        /// <summary>
        /// The code of the Int16.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const char Code = 'n';
        /// <summary>
        /// The signature of the Int16.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const string Signature = "n";
    }

    /// <summary>
    /// The type of an unsigned Int16.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public class UInt16Type
    {
        /// <summary>
        /// The code of the unsigned Int16.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const char Code = 'q';
        /// <summary>
        /// The signature of the unsigned Int16.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const string Signature = "q";
    }

    /// <summary>
    /// The type of a Int32.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public class Int32Type
    {
        /// <summary>
        /// The code of the Int32.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const char Code = 'i';
        /// <summary>
        /// The signature of the Int32.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const string Signature = "i";
    }

    /// <summary>
    /// The type of an unsigned Int32.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public class UInt32Type
    {
        /// <summary>
        /// The code of the unsigned Int32.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const char Code = 'u';
        /// <summary>
        /// The signature of the unsigned Int32.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const string Signature = "u";
    }

    /// <summary>
    /// The type of a Int64.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public class Int64Type
    {
        /// <summary>
        /// The code of the Int64.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const char Code = 'x';
        /// <summary>
        /// The signature of the Int64.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const string Signature = "x";
    }

    /// <summary>
    /// The type of an unsigned Int64.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public class UInt64Type
    {
        /// <summary>
        /// The code of the unsigned Int64.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const char Code = 't';
        /// <summary>
        /// The signature of the unsigned Int64.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const string Signature = "t";
    }

    /// <summary>
    /// The type of the double.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public class DoubleType
    {
        /// <summary>
        /// The code of the double.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const char Code = 'd';
        /// <summary>
        /// The signature of the double.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const string Signature = "d";
    }

    /// <summary>
    /// The type of a string.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public class StringType
    {
        /// <summary>
        /// The code of the string.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const char Code = 's';
        /// <summary>
        /// The signature of the string.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const string Signature = "s";
    }

    /// <summary>
    /// The type of an object path.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public class ObjectPathType
    {
        /// <summary>
        /// The code of the object path.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const char Code = 'o';
        /// <summary>
        /// The signature of the object path.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const string Signature = "o";
    }

    /// <summary>
    /// The type of a signature.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public class SignatureType
    {
        /// <summary>
        /// The code of the signature.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const char Code = 'g';
        /// <summary>
        /// The signature of the signature.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const string Signature = "g";
    }

    /// <summary>
    /// The type of a array.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public class ArrayType
    {
        /// <summary>
        /// The code of the array.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const char Code = 'a';
        /// <summary>
        /// The signature of the array.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const string Signature = "a";
    }

    /// <summary>
    /// The type of a struct.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public class StructType
    {
        /// <summary>
        /// The code of the struct.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const char Code = 'r';
        /// <summary>
        /// The signature of the struct.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const string Signature = "r";
    }

    /// <summary>
    /// The type of a variant.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public class VariantType
    {
        /// <summary>
        /// The code of the variant.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const char Code = 'v';
        /// <summary>
        /// The signature of the variant.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const string Signature = "v";
    }

    /// <summary>
    /// The type of a dictionary.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public class DictEntryType
    {
        /// <summary>
        /// The code of the dictionary.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const char Code = 'e';
        /// <summary>
        /// The signature of the dictionary.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const string Signature = "e";
    }

    /// <summary>
    /// The type of an unix file descriptor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public class UnixFdType
    {
        /// <summary>
        /// The code of unix fd.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const char Code = 'h';
        /// <summary>
        /// The signature of the unix fd.
        /// <para>Since EFL 1.23.</para>
        /// </summary>
        public const string Signature = "h";
    }

//     public static readonly ByteType       ByteT       = new ByteType();
//     public static readonly BooleanType    BooleanT    = new BooleanType();
//     public static readonly Int16Type      Int16T      = new Int16Type();
//     public static readonly UInt16Type     UInt16T     = new UInt16Type();
//     public static readonly Int32Type      Int32T      = new Int32Type();
//     public static readonly UInt32Type     UInt32T     = new UInt32Type();
//     public static readonly Int64Type      Int64T      = new Int64Type();
//     public static readonly UInt64Type     UInt64T     = new UInt64Type();
//     public static readonly DoubleType     DoubleT     = new DoubleType();
//     public static readonly StringType     StringT     = new StringType();
//     public static readonly ObjectPathType ObjectPathT = new ObjectPathType();
//     public static readonly SignatureType  SignatureT  = new SignatureType();
//     public static readonly ArrayType      ArrayT      = new ArrayType();
//     public static readonly StructType     StructT     = new StructType();
//     public static readonly VariantType    VariantT    = new VariantType();
//     public static readonly DictEntryType  DictEntryT  = new DictEntryType();
//     public static readonly UnixFdType     UnixFdT     = new UnixFdType();
//
//     public static readonly ByteType       y = ByteT;
//     public static readonly BooleanType    b = BooleanT;
//     public static readonly Int16Type      n = Int16T;
//     public static readonly UInt16Type     q = UInt16T;
//     public static readonly Int32Type      i = Int32T;
//     public static readonly UInt32Type     u = UInt32T;
//     public static readonly Int64Type      x = Int64T;
//     public static readonly UInt64Type     t = UInt64T;
//     public static readonly DoubleType     d = DoubleT;
//     public static readonly StringType     s = StringT;
//     public static readonly ObjectPathType o = ObjectPathT;
//     public static readonly SignatureType  g = SignatureT;
//     public static readonly ArrayType      a = ArrayT;
//     public static readonly StructType     r = StructT;
//     public static readonly VariantType    v = VariantT;
//     public static readonly DictEntryType  e = DictEntryT;
//     public static readonly UnixFdType     h = UnixFdT;
}

/// <summary>
/// Arguments to a basic message eldbus.
/// <para>Since EFL 1.23.</para>
/// </summary>
public abstract class BasicMessageArgument
{
    /// <summary>
    /// Appends a message to eldbus.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="msg">The message to be appended.</param>
    public void AppendTo(eldbus.Message msg)
    {
        if (!InternalAppendTo(msg))
        {
            throw new SEHException("Eldbus: could not append basic type to eldbus.Message");
        }
    }

    /// <summary>
    /// Appends a message to eldbus.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="iter">The messages to append.</param>
    public void AppendTo(eldbus.MessageIterator iter)
    {
        if (!InternalAppendTo(iter))
        {
            throw new SEHException("Eldbus: could not append basic type to eldbus.MessageIterator");
        }
    }

    /// <summary>
    /// The code of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public abstract char TypeCode {get;}

    /// <summary>
    /// The signature of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public abstract string Signature {get;}

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected abstract bool InternalAppendTo(eldbus.Message msg);

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected abstract bool InternalAppendTo(eldbus.MessageIterator iter);

    /// <summary>
    /// Conversion operator of BasicMessageArgument from byte.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The byte to be converted.</param>
    public static implicit operator BasicMessageArgument(byte arg)
    {
        return new ByteMessageArgument(arg);
    }

    /// <summary>
    /// Conversion operator of BasicMessageArgument from bool.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The bool to be converted.</param>
    public static implicit operator BasicMessageArgument(bool arg)
    {
        return new BoolMessageArgument(arg);
    }

    /// <summary>
    /// Conversion operator of BasicMessageArgument from Int16.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The int16 to be converted.</param>
    public static implicit operator BasicMessageArgument(Int16 arg)
    {
        return new Int16MessageArgument(arg);
    }

    /// <summary>
    /// Conversion operator of BasicMessageArgument from unsigned int16.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The unsigned int16 to be converted.</param>
    public static implicit operator BasicMessageArgument(UInt16 arg)
    {
        return new UInt16MessageArgument(arg);
    }

    /// <summary>
    /// Conversion operator of BasicMessageArgument from int32.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The int32 to be converted.</param>
    public static implicit operator BasicMessageArgument(Int32 arg)
    {
        return new Int32MessageArgument(arg);
    }

    /// <summary>
    /// Conversion operator of BasicMessageArgument from unsigned int32.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The unsigned int32 to be converted.</param>
    public static implicit operator BasicMessageArgument(UInt32 arg)
    {
        return new UInt32MessageArgument(arg);
    }

    /// <summary>
    /// Conversion operator of BasicMessageArgument from int64.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The int64 to be converted.</param>
    public static implicit operator BasicMessageArgument(Int64 arg)
    {
        return new Int64MessageArgument(arg);
    }

    /// <summary>
    /// Conversion operator of BasicMessageArgument from unsigned int64.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The unsigned int64 to be converted.</param>
    public static implicit operator BasicMessageArgument(UInt64 arg)
    {
        return new UInt64MessageArgument(arg);
    }

    /// <summary>
    /// Conversion operator of BasicMessageArgument from string.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">the string to be converted.</param>
    public static implicit operator BasicMessageArgument(string arg)
    {
        return new StringMessageArgument(arg);
    }

    /// <summary>
    /// Conversion operator of BasicMessageArgument from signature.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The signature to be converted.</param>
    public static implicit operator BasicMessageArgument(SignatureString arg)
    {
        return new SignatureMessageArgument(arg);
    }

    /// <summary>
    /// Conversion operator of BasicMessageArgument from object path.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The object path to be converted.</param>
    public static implicit operator BasicMessageArgument(ObjectPath arg)
    {
        return new ObjectPathMessageArgument(arg);
    }

    /// <summary>
    /// Conversion operator of BasicMessageArgument from unix fd.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The unix fd to be converted.</param>
    public static implicit operator BasicMessageArgument(UnixFd arg)
    {
        return new UnixFdMessageArgument(arg);
    }
}

/// <summary>
/// Arguments to a byte message eldbus.
/// <para>Since EFL 1.23.</para>
/// </summary>
public class ByteMessageArgument : BasicMessageArgument
{
    private byte value;

    /// <summary>
    /// Constructor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The argument of the eldbus.</param>
    public ByteMessageArgument(byte arg)
    {
        value = arg;
    }

    /// <summary>
    /// The code of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override char TypeCode
    {
        get { return Argument.ByteType.Code; }
    }

    /// <summary>
    /// The signature of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override string Signature
    {
        get { return Argument.ByteType.Signature; }
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.Message msg)
    {
        return eldbus_message_arguments_append(msg.Handle, Signature, value);
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.MessageIterator iter)
    {
        return eldbus_message_iter_basic_append(iter.Handle, TypeCode, value);
    }
}

/// <summary>
/// Arguments to a bool message eldbus.
/// <para>Since EFL 1.23.</para>
/// </summary>
public class BoolMessageArgument : BasicMessageArgument
{
    private Int32 value;

    /// <summary>
    /// Constructor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The arguments of the eldbus.</param>
    public BoolMessageArgument(bool arg)
    {
        value = Convert.ToInt32(arg);
    }

    /// <summary>
    /// The code of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override char TypeCode
    {
        get { return Argument.BooleanType.Code; }
    }

    /// <summary>
    /// The signature of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override string Signature
    {
        get { return Argument.ByteType.Signature; }
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.Message msg)
    {
        return eldbus_message_arguments_append(msg.Handle, Signature, value);
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.MessageIterator iter)
    {
        return eldbus_message_iter_basic_append(iter.Handle, TypeCode, value);
    }
}

/// <summary>
/// Arguments to an int16 message eldbus.
/// <para>Since EFL 1.23.</para>
/// </summary>
public class Int16MessageArgument : BasicMessageArgument
{
    private Int16 value;

    /// <summary>
    /// Constructor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The arguments of the eldbus.</param>
    public Int16MessageArgument(Int16 arg)
    {
        value = arg;
    }

    /// <summary>
    /// The code of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override char TypeCode
    {
        get { return Argument.Int16Type.Code; }
    }

    /// <summary>
    /// The signature of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override string Signature
    {
        get { return Argument.ByteType.Signature; }
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.Message msg)
    {
        return eldbus_message_arguments_append(msg.Handle, Signature, value);
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.MessageIterator iter)
    {
        return eldbus_message_iter_basic_append(iter.Handle, TypeCode, value);
    }
}

/// <summary>
/// Arguments to an unsigned int16 message eldbus.
/// <para>Since EFL 1.23.</para>
/// </summary>
public class UInt16MessageArgument : BasicMessageArgument
{
    private UInt16 value;

    /// <summary>
    /// Constructor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">the arguments of the eldbus.</param>
    public UInt16MessageArgument(UInt16 arg)
    {
        value = arg;
    }

    /// <summary>
    /// The code of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override char TypeCode
    {
        get { return Argument.UInt16Type.Code; }
    }

    /// <summary>
    /// The signature of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override string Signature
    {
        get { return Argument.ByteType.Signature; }
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.Message msg)
    {
        return eldbus_message_arguments_append(msg.Handle, Signature, value);
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.MessageIterator iter)
    {
        return eldbus_message_iter_basic_append(iter.Handle, TypeCode, value);
    }
}

/// <summary>
/// Arguments to an int32 message eldbus.
/// <para>Since EFL 1.23.</para>
/// </summary>
public class Int32MessageArgument : BasicMessageArgument
{
    private Int32 value;

    /// <summary>
    /// Constructor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The arguments of the eldbus.</param>
    public Int32MessageArgument(Int32 arg)
    {
        value = arg;
    }

    /// <summary>
    /// The code of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override char TypeCode
    {
        get { return Argument.Int32Type.Code; }
    }

    /// <summary>
    /// The signature of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override string Signature
    {
        get { return Argument.ByteType.Signature; }
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.Message msg)
    {
        return eldbus_message_arguments_append(msg.Handle, Signature, value);
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.MessageIterator iter)
    {
        return eldbus_message_iter_basic_append(iter.Handle, TypeCode, value);
    }
}

/// <summary>
/// Arguments to an unsigned int32 message eldbus.
/// <para>Since EFL 1.23.</para>
/// </summary>
public class UInt32MessageArgument : BasicMessageArgument
{
    private UInt32 value;

    /// <summary>
    /// Constructor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The arguments of the unsigned type.</param>
    public UInt32MessageArgument(UInt32 arg)
    {
        value = arg;
    }

    /// <summary>
    /// The code of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override char TypeCode
    {
        get { return Argument.UInt32Type.Code; }
    }

    /// <summary>
    /// The signature of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override string Signature
    {
        get { return Argument.ByteType.Signature; }
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.Message msg)
    {
        return eldbus_message_arguments_append(msg.Handle, Signature, value);
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.MessageIterator iter)
    {
        return eldbus_message_iter_basic_append(iter.Handle, TypeCode, value);
    }
}

/// <summary>
/// Arguments to an int64 message eldbus.
/// <para>Since EFL 1.23.</para>
/// </summary>
public class Int64MessageArgument : BasicMessageArgument
{
    private Int64 value;

    /// <summary>
    /// Constructor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The arguments of the eldbus.</param>
    public Int64MessageArgument(Int64 arg)
    {
        value = arg;
    }

    /// <summary>
    /// The code of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override char TypeCode
    {
        get { return Argument.Int64Type.Code; }
    }

    /// <summary>
    /// The signature of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override string Signature
    {
        get { return Argument.ByteType.Signature; }
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.Message msg)
    {
        return eldbus_message_arguments_append(msg.Handle, Signature, value);
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.MessageIterator iter)
    {
        return eldbus_message_iter_basic_append(iter.Handle, TypeCode, value);
    }
}

/// <summary>
/// Arguments to an unsigned int64 message eldbus.
/// <para>Since EFL 1.23.</para>
/// </summary>
public class UInt64MessageArgument : BasicMessageArgument
{
    private UInt64 value;

    /// <summary>
    /// Constructor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The arguments of the eldbus.</param>
    public UInt64MessageArgument(UInt64 arg)
    {
        value = arg;
    }

    /// <summary>
    /// The code of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override char TypeCode
    {
        get { return Argument.UInt64Type.Code; }
    }

    /// <summary>
    /// The signature of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override string Signature
    {
        get { return Argument.ByteType.Signature; }
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.Message msg)
    {
        return eldbus_message_arguments_append(msg.Handle, Signature, value);
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.MessageIterator iter)
    {
        return eldbus_message_iter_basic_append(iter.Handle, TypeCode, value);
    }
}

/// <summary>
/// Arguments to a double message eldbus.
/// <para>Since EFL 1.23.</para>
/// </summary>
public class DoubleMessageArgument : BasicMessageArgument
{
    private double value;

    /// <summary>
    /// Constructor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The arguments of the eldbus.</param>
    public DoubleMessageArgument(double arg)
    {
        value = arg;
    }

    /// <summary>
    /// The code of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override char TypeCode
    {
        get { return Argument.DoubleType.Code; }
    }

    /// <summary>
    /// The signature of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override string Signature
    {
        get { return Argument.ByteType.Signature; }
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.Message msg)
    {
        return eldbus_message_arguments_append(msg.Handle, Signature, value);
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.MessageIterator iter)
    {
        return eldbus_message_iter_basic_append(iter.Handle, TypeCode, value);
    }
}

/// <summary>
/// Arguments to a string like message eldbus.
/// <para>Since EFL 1.23.</para>
/// </summary>
public abstract class StringLikeMessageArgument : BasicMessageArgument
{
    private string value;

    /// <summary>
    /// Constructor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The arguments of the eldbus.</param>
    public StringLikeMessageArgument(string arg)
    {
        value = arg;
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.Message msg)
    {
        return eldbus_message_arguments_append(msg.Handle, Signature, value);
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.MessageIterator iter)
    {
        return eldbus_message_iter_basic_append(iter.Handle, TypeCode, value);
    }
}

/// <summary>
/// Arguments to a string message eldbus.
/// <para>Since EFL 1.23.</para>
/// </summary>
public class StringMessageArgument : StringLikeMessageArgument
{
    /// <summary>
    /// Constructor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The arguments of the eldbus.</param>
    public StringMessageArgument(string arg) : base(arg)
    {
    }

    /// <summary>
    /// The code of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override char TypeCode
    {
        get { return Argument.StringType.Code; }
    }

    /// <summary>
    /// The signature of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override string Signature
    {
        get { return Argument.ByteType.Signature; }
    }
}

/// <summary>
/// Arguments to an object path message eldbus.
/// <para>Since EFL 1.23.</para>
/// </summary>
public class ObjectPathMessageArgument : StringLikeMessageArgument
{
    /// <summary>
    /// Constructor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg"></param>
    public ObjectPathMessageArgument(ObjectPath arg) : base(arg.value)
    {
    }

    /// <summary>
    /// The code of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override char TypeCode
    {
        get { return Argument.ObjectPathType.Code; }
    }

    /// <summary>
    /// The signature of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override string Signature
    {
        get { return Argument.ByteType.Signature; }
    }
}

/// <summary>
/// Arguments to a signature message eldbus.
/// <para>Since EFL 1.23.</para>
/// </summary>
public class SignatureMessageArgument : StringLikeMessageArgument
{
    /// <summary>
    /// Constructor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The arguments of the eldbus.</param>
    public SignatureMessageArgument(SignatureString arg) : base(arg.value)
    {
    }

    /// <summary>
    /// The code of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override char TypeCode
    {
        get { return Argument.SignatureType.Code; }
    }

    /// <summary>
    /// The signature of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override string Signature
    {
        get { return Argument.ByteType.Signature; }
    }
}

/// <summary>
/// Arguments to an unixfd message eldbus.
/// <para>Since EFL 1.23.</para>
/// </summary>
public class UnixFdMessageArgument : BasicMessageArgument
{
    private Int32 value;

    /// <summary>
    /// Constructor.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="arg">The arguments of the eldbus.</param>
    public UnixFdMessageArgument(UnixFd arg)
    {
        value = arg.value;
    }

    /// <summary>
    /// The code of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override char TypeCode
    {
        get { return Argument.UnixFdType.Code; }
    }

    /// <summary>
    /// The signature of the type.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public override string Signature
    {
        get { return Argument.ByteType.Signature; }
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.Message msg)
    {
        return eldbus_message_arguments_append(msg.Handle, Signature, value);
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    protected override bool InternalAppendTo(eldbus.MessageIterator iter)
    {
        return eldbus_message_iter_basic_append(iter.Handle, TypeCode, value);
    }
}

/// <summary>
/// Function type to delegate a message.
/// <para>Since EFL 1.23.</para>
/// </summary>
/// <param name="msg">The message.</param>
/// <param name="pending"></param>
public delegate void MessageDelegate(eldbus.Message msg, eldbus.Pending pending);

/// <summary>
/// Commons for eldbus.
/// <para>Since EFL 1.23.</para>
/// </summary>
public static class Common
{
    /// <summary>
    /// Register the NullError.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public static void RaiseNullHandle()
    {
        if (NullHandleError == 0)
        {
            NullHandleError = Eina.Error.Register("Eldbus: null handle");
        }

        Eina.Error.Raise(NullHandleError);
    }

    /// <summary>
    /// Instance for a EldBus_Message_Cb.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="data">The data to the eldbus.</param>
    /// <param name="msg">The message to eldbus.</param>
    /// <param name="pending"></param>
    public delegate void Eldbus_Message_Cb(IntPtr data, IntPtr msg, IntPtr pending);

    /// <summary>
    /// Get a wrapper for the message instance.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public static IntPtr GetMessageCbWrapperPtr()
    {
        return Marshal.GetFunctionPointerForDelegate(GetMessageCbWrapper());
    }

    /// <summary>
    /// Gets the message wrapper.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    public static Eldbus_Message_Cb GetMessageCbWrapper()
    {
        if (message_cb_wrapper == null)
        {
            message_cb_wrapper = new Eldbus_Message_Cb(MessageCbWrapper);
        }

        return message_cb_wrapper;
    }

    /// <summary>
    ///  Wraps the data to a message.
    /// <para>Since EFL 1.23.</para>
    /// </summary>
    /// <param name="data"></param>
    /// <param name="msg_hdl"></param>
    /// <param name="pending_hdl"></param>
    public static void MessageCbWrapper(IntPtr data, IntPtr msg_hdl, IntPtr pending_hdl)
    {
        MessageDelegate dlgt = Marshal.GetDelegateForFunctionPointer(data, typeof(MessageDelegate)) as MessageDelegate;
        if (dlgt == null)
        {
            Eina.Log.Error("Eldbus: invalid delegate pointer from Eldbus_Message_Cb");
            return;
        }

        eldbus.Message msg;
        eldbus.Pending pending;

        try
        {
            msg = new eldbus.Message(msg_hdl, false);
            pending = new eldbus.Pending(pending_hdl, false);
        }
        catch (Exception e)
        {
            Eina.Log.Error("Eldbus: could not convert Eldbus_Message_Cb parameters. Exception: " + e.ToString());
            return;
        }

        try
        {
            dlgt(msg, pending);
        }
        catch (Exception e)
        {
            Eina.Log.Error("Eldbus: Eldbus_Message_Cb delegate error. Exception: " + e.ToString());
        }
    }

    private static Eldbus_Message_Cb message_cb_wrapper = null;
    private static Eina.Error NullHandleError = 0;
}

}
