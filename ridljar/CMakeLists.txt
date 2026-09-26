# This file is part of the LibreStudio project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# ridljar -- the real UNO Java bridge classes. Three targets, same as
# upstream's CustomTarget_javamaker.mk + Jar_unoloader.mk + Jar_libreoffice.mk
# (Jar_ridl.mk is NOT ported: like unoil.jar/jurt.jar/juh.jar it has zero
# gb_Jar_add_sourcefile calls -- it is an empty legacy Class-Path shim jar
# that nothing in this port references).

# ridljar/javamaker: CustomTarget_javamaker.mk -- javac stubs for udkapi only
# (no -X extras, unlike unoil's offapi-primary + udkapi-extra call), packaged
# straight into libreoffice.jar below via PACKAGEDIRS.
librestudio_uno_api_java(ridljar_javamaker
    RDB        "${udkapi_RDB}"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/javamaker")

# unoloader.jar: Jar_unoloader.mk -- the tiny bootstrap loader, no
# customtargets/jars/externals, manifest is just "Sealed: true".
librestudio_add_uno_jar(unoloader
    PACKAGEROOT "com"
    MANIFEST    "${CMAKE_SOURCE_DIR}/ridljar/source/unoloader/com/sun/star/lib/unoloader/manifest"
    SOURCES
        ridljar/source/unoloader/com/sun/star/lib/unoloader/UnoClassLoader
        ridljar/source/unoloader/com/sun/star/lib/unoloader/UnoLoader)

# libreoffice.jar: Jar_libreoffice.mk -- the real payload. Compile classpath
# and final packagedirs both need ridljar/javamaker's and unoil/javamaker's
# generated stub trees (com.sun.star.uno.XInterface, com.sun.star.lang.XComponent,
# etc. -- distinct simple names from the hand-written UnoRuntime/Any/Type/etc.
# below, so the two class sets coexist as siblings under the same com/ tree,
# merged by the final jar -u passes); compile classpath additionally needs
# unoloader.jar (BridgeFactory.java forwards to it) and java_websocket.jar
# (WebsocketConnection.java) from RepositoryExternal.mk's
# gb_Jar__use_java_websocket. Manifest Class-Path is the three entries
# gb_Jar_add_manifest_classpath lists, resolved for this Linux-only port
# ($(if $(filter MACOSX,$(OS)),../../Frameworks/,../) --> ../).
librestudio_add_uno_jar(libreoffice
    PACKAGEROOT "com"
    MANIFEST    "${CMAKE_SOURCE_DIR}/ridljar/util/manifest"
    MANIFEST_CLASSPATH
        java_websocket.jar
        unoloader.jar
        ../
    CLASSPATH
        "${ridljar_javamaker_JAVA_OUTPUT_DIR}"
        "${unoil_JAVA_OUTPUT_DIR}"
        "${unoloader_JAR}"
        "${JAVA_WEBSOCKET_JAR}"
    PACKAGEDIRS
        "${ridljar_javamaker_JAVA_OUTPUT_DIR}/com"
        "${unoil_JAVA_OUTPUT_DIR}/com"
    DEPENDS
        ${ridljar_javamaker_JAVA_TARGET}
        ${unoil_JAVA_TARGET}
        unoloader_jar
        java_websocket_ext
    SOURCES
        ridljar/com/sun/star/comp/bridgefactory/BridgeFactory
        ridljar/com/sun/star/comp/connections/Acceptor
        ridljar/com/sun/star/comp/connections/Connector
        ridljar/com/sun/star/comp/connections/ConstantInstanceProvider
        ridljar/com/sun/star/comp/connections/Implementation
        ridljar/com/sun/star/comp/connections/PipedConnection
        ridljar/com/sun/star/comp/helper/Bootstrap
        ridljar/com/sun/star/comp/helper/BootstrapException
        ridljar/com/sun/star/comp/helper/ComponentContext
        ridljar/com/sun/star/comp/helper/ComponentContextEntry
        ridljar/com/sun/star/comp/helper/SharedLibraryLoader
        ridljar/com/sun/star/comp/loader/FactoryHelper
        ridljar/com/sun/star/comp/loader/JavaLoader
        ridljar/com/sun/star/comp/loader/JavaLoaderFactory
        ridljar/com/sun/star/comp/loader/RegistrationClassFinder
        ridljar/com/sun/star/comp/servicemanager/ServiceManager
        ridljar/com/sun/star/comp/urlresolver/UrlResolver
        ridljar/com/sun/star/lib/connections/pipe/PipeConnection
        ridljar/com/sun/star/lib/connections/pipe/pipeAcceptor
        ridljar/com/sun/star/lib/connections/pipe/pipeConnector
        ridljar/com/sun/star/lib/connections/socket/ConnectionDescriptor
        ridljar/com/sun/star/lib/connections/socket/SocketConnection
        ridljar/com/sun/star/lib/connections/socket/socketAcceptor
        ridljar/com/sun/star/lib/connections/socket/socketConnector
        ridljar/com/sun/star/lib/connections/websocket/ConnectionDescriptor
        ridljar/com/sun/star/lib/connections/websocket/WebsocketConnection
        ridljar/com/sun/star/lib/connections/websocket/websocketConnector
        ridljar/com/sun/star/lib/uno/Proxy
        ridljar/com/sun/star/lib/uno/adapter/ByteArrayToXInputStreamAdapter
        ridljar/com/sun/star/lib/uno/adapter/InputStreamToXInputStreamAdapter
        ridljar/com/sun/star/lib/uno/adapter/OutputStreamToXOutputStreamAdapter
        ridljar/com/sun/star/lib/uno/adapter/XInputStreamToInputStreamAdapter
        ridljar/com/sun/star/lib/uno/adapter/XOutputStreamToByteArrayAdapter
        ridljar/com/sun/star/lib/uno/adapter/XOutputStreamToOutputStreamAdapter
        ridljar/com/sun/star/lib/uno/bridges/java_remote/BridgedObject
        ridljar/com/sun/star/lib/uno/bridges/java_remote/ProxyFactory
        ridljar/com/sun/star/lib/uno/bridges/java_remote/RequestHandler
        ridljar/com/sun/star/lib/uno/bridges/java_remote/XConnectionInputStream_Adapter
        ridljar/com/sun/star/lib/uno/bridges/java_remote/XConnectionOutputStream_Adapter
        ridljar/com/sun/star/lib/uno/bridges/java_remote/java_remote_bridge
        ridljar/com/sun/star/lib/uno/environments/java/java_environment
        ridljar/com/sun/star/lib/uno/environments/remote/IProtocol
        ridljar/com/sun/star/lib/uno/environments/remote/IReceiver
        ridljar/com/sun/star/lib/uno/environments/remote/IThreadPool
        ridljar/com/sun/star/lib/uno/environments/remote/JavaThreadPool
        ridljar/com/sun/star/lib/uno/environments/remote/JavaThreadPoolFactory
        ridljar/com/sun/star/lib/uno/environments/remote/Job
        ridljar/com/sun/star/lib/uno/environments/remote/JobQueue
        ridljar/com/sun/star/lib/uno/environments/remote/Message
        ridljar/com/sun/star/lib/uno/environments/remote/NativeThreadPool
        ridljar/com/sun/star/lib/uno/environments/remote/ThreadId
        ridljar/com/sun/star/lib/uno/environments/remote/ThreadPoolManager
        ridljar/com/sun/star/lib/uno/environments/remote/remote_environment
        ridljar/com/sun/star/lib/uno/helper/ComponentBase
        ridljar/com/sun/star/lib/uno/helper/Factory
        ridljar/com/sun/star/lib/uno/helper/InterfaceContainer
        ridljar/com/sun/star/lib/uno/helper/MultiTypeInterfaceContainer
        ridljar/com/sun/star/lib/uno/helper/PropertySet
        ridljar/com/sun/star/lib/uno/helper/PropertySetMixin
        ridljar/com/sun/star/lib/uno/helper/UnoUrl
        ridljar/com/sun/star/lib/uno/helper/WeakAdapter
        ridljar/com/sun/star/lib/uno/helper/WeakBase
        ridljar/com/sun/star/lib/uno/protocols/urp/Cache
        ridljar/com/sun/star/lib/uno/protocols/urp/Marshal
        ridljar/com/sun/star/lib/uno/protocols/urp/PendingRequests
        ridljar/com/sun/star/lib/uno/protocols/urp/Unmarshal
        ridljar/com/sun/star/lib/uno/protocols/urp/UrpMessage
        ridljar/com/sun/star/lib/uno/protocols/urp/urp
        ridljar/com/sun/star/lib/uno/typedesc/FieldDescription
        ridljar/com/sun/star/lib/uno/typedesc/MemberDescriptionHelper
        ridljar/com/sun/star/lib/uno/typedesc/MethodDescription
        ridljar/com/sun/star/lib/uno/typedesc/TypeDescription
        ridljar/com/sun/star/lib/uno/typeinfo/AttributeTypeInfo
        ridljar/com/sun/star/lib/uno/typeinfo/ConstantTypeInfo
        ridljar/com/sun/star/lib/uno/typeinfo/MemberTypeInfo
        ridljar/com/sun/star/lib/uno/typeinfo/MethodTypeInfo
        ridljar/com/sun/star/lib/uno/typeinfo/ParameterTypeInfo
        ridljar/com/sun/star/lib/uno/typeinfo/TypeInfo
        ridljar/com/sun/star/lib/util/AsynchronousFinalizer
        ridljar/com/sun/star/lib/util/DisposeListener
        ridljar/com/sun/star/lib/util/DisposeNotifier
        ridljar/com/sun/star/lib/util/NativeLibraryLoader
        ridljar/com/sun/star/lib/util/StringHelper
        ridljar/com/sun/star/lib/util/UrlToFileMapper
        ridljar/com/sun/star/lib/util/WeakMap
        ridljar/com/sun/star/uno/Any
        ridljar/com/sun/star/uno/AnyConverter
        ridljar/com/sun/star/uno/Ascii
        ridljar/com/sun/star/uno/AsciiString
        ridljar/com/sun/star/uno/Enum
        ridljar/com/sun/star/uno/IBridge
        ridljar/com/sun/star/uno/IEnvironment
        ridljar/com/sun/star/uno/IMapping
        ridljar/com/sun/star/uno/IQueryInterface
        ridljar/com/sun/star/uno/MappingException
        ridljar/com/sun/star/uno/Type
        ridljar/com/sun/star/uno/UnoRuntime
        ridljar/com/sun/star/uno/WeakReference)

# install(): both URE and OOO layer jars resolve to the same "program/classes"
# on this Linux-only port (configure.ac's LIBO_SHARE_JAVA_FOLDER /
# LIBO_URE_SHARE_JAVA_FOLDER, non-Darwin/non-WNT branch), matching
# gb_Helper_register_jars_for_install,URE,ure,[libreoffice unoloader ...]
# in Repository.mk.
install(FILES "${unoloader_JAR}" "${libreoffice_JAR}" DESTINATION program/classes)
