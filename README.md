# JVM-Native-Classdumping
Write-up on natively dumping classes by hooking ClassLoader#defineClass

# Hooking class-loading for java-applications natively

Because someone has been running around with this epic knowledge about the JVM (without actually sharing his/her code), I've decided to make sure I have something to do during my vacation. Now because I've chosen something with the JVM again (sadly) and I'm a big advocate of open-source projects (even when it comes to cheating) I thought: "Why the fuck not steal this dude's idea, and make it open-source?".

So that's exactly what I've done.

# Where to start?

Since classloading starts at, you guessed it: the classloader class, that's where we will start looking. So, lets look at classloading in Java. If you want to load a class in java, you'd most likely do it like this:

    ClassLoader classLoader = new MyClassLoader(); /* lets keep it small */
    Class<MyLoadedClass> myClass = classLoader.loadClass("MyLoadedClass"); /* or use defineClass */
    ....

Since ClassLoader#loadClass inevitably leads to defineClass, let's take a look at that. [ClassLoader.class](http://hg.openjdk.java.net/jdk8/jdk8/jdk/file/687fd7c7986d/src/share/classes/java/lang/ClassLoader.java)

     protected final Class<?> defineClass(String name, byte[] b, int off, int len,
                                         ProtectionDomain protectionDomain)
        throws ClassFormatError
    {
        protectionDomain = preDefineClass(name, protectionDomain);
        String source = defineClassSourceLocation(protectionDomain);
        Class<?> c = defineClass1(name, b, off, len, protectionDomain, source); /* epic naming btw, million dollar company at work here */
        postDefineClass(c, protectionDomain);
        return c;
    }

Alright, as we can see, most functions actually don't use the buffer at all (which is a bummer, because we could've done a LOT more) except defineClass1.
So we might as well take a look at that.

    private native Class<?> defineClass1(String name, byte[] b, int off, int len,
                                         ProtectionDomain pd, String source);

Hahahaha yes! A native function, finally. This means we can finally mess with some stuff the JVM flushes down the drain. Following common native method naming, and knowing the JVM doesn't register native functions manually, we can deduce that the native name of our defineClass1 function would amount to something like this: `Java_java_lang_ClassLoader_defineClass1` and taking a look at the java.dll confirms our suspicion, where we find this method:

    __int64 __fastcall Java_java_lang_ClassLoader_defineClass1(__int64 a1, __int64 a2, char *a3, __int64 a4, unsigned int a5, unsigned int a6, __int64 a7, char *Memory)

Now by taking a wild guess, we can come to the conclusion that, a1 is the Java enviroment, and a2 is the ClassLoader object.
Taking a look at the Java call of the native method

    Class<?> c = defineClass1(name, b, off, len, protectionDomain, source);

This would also confirm our suspicion, since the third argument natively is in this case, the name. where a5 is a pointer to the buffer. So the new method would be:

    __int64 __fastcall Java_java_lang_ClassLoader_defineClass1(JavaEnv *env, jobject obj, jstring name, jbyteArray buffer, jint offset, jint length, jobject protection_domain, jstring src)

Great! Let's hook this function, steal the bytes, and dip!

....

Continuation following soon.
