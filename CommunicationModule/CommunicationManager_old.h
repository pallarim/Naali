#ifndef incl_CommunicationManager_h
#define incl_CommunicationManager_h

#include "StableHeaders.h"

#include "Foundation.h"
#include "EventDataInterface.h"

#include "IMMessage.h"
#include "Contact.h"
#include "PresenceStatus.h"
#include "IMSession.h"
#include "CommunicationEvents.h"
#include "Participant.h"
#include "FriendRequest.h"

#include "glib.h"

/**
 * Implementation of CommunicationServiceInterface
 *
 * Uses telepathy-python library as backend using IMDemo python module through PythonScriptModule.
 * Current python implementation uses Telepathy-Gabble connection manager to provice Jabber IM 
 * services. By nature of Telepathy framework it's quite easy to use any connection manager and 
 * the IM services those offers. Communication Service is designed to be generic IM service interface
 * which hide protocol specific features from user.
 *
 * @todo Make this threadsafe
 * @todo paramters are char* in python function calls. Maybe eg. shared_prt or std::string would be better? (But this is python module issue)
 * @todo Move console commands to separate class: ConsoleUI
 * @todo namespace should be renamed to "CommunicationImpl" to hide the implementation from namespace of service interface
 */

namespace Communication
{
	#define COMMUNICATION_PYTHON_MODULE "communication.IMDemo" 
	#define COMMUNICATION_PYTHON_CLASS "IMDemo"
	#define LOCAL_USER_ID "1" // defined in python side

	class Farsight2Thread
	{
	public:
//		Farsight2Thread();
//		~Farsight2Thread();
		void operator()();
		void Stop();

	private:
		GMainLoop* gmain_loop_;
	};


	class DBusDaemonThread
	{
	public:
		void operator()();
		void Stop();
	};


	/**
	 *  Implements CommunicationServiceInterface with python backend which uses telepathy-python
	 */
	class CommunicationManager : public CommunicationServiceInterface
	{
		friend class IMSession;
		friend class Session;
		friend class FriendRequest;
		friend class PresenceStatus;

		MODULE_LOGGING_FUNCTIONS
			
	public:
		CommunicationManager(Foundation::Framework *f);
		~CommunicationManager(void);

		static const std::string NameStatic() { return "CommunicationManager"; } // for logging functionality

		// CommunicationServiceInterface begin
		void OpenConnection(CommunicationSettingsInterfacePtr c);

		// Close exist connection to IM server
		void CloseConnection();
		IMSessionPtr CreateIMSession(ContactPtr contact);
		IMSessionPtr CreateIMSession(ContactInfoPtr contact);
		ContactListPtr GetContactList() const;
		void SetPresenceStatus(PresenceStatusPtr p);
		PresenceStatusPtr GetPresenceStatus();
		IMMessagePtr CreateIMMessage(std::string text);
		void SendFriendRequest(ContactInfoPtr contact_info);
		void RemoveContact(ContactPtr contact); 
		//virtual CredentialsPtr GetCredentials(); 
        virtual CommunicationSettingsInterfacePtr GetCommunicationSettings(); 

        virtual void CreateAccount();

		bool IsInitialized() { return initialized_; }
		void UnInitialize();
		
		// CommunicationServiceInterface end

		// callbacks for console commands
		Console::CommandResult ConsoleHelp(const Core::StringVector &params);
		Console::CommandResult ConsoleState(const Core::StringVector &params);
        Console::CommandResult ConsoleLogin(const Core::StringVector &params);
		Console::CommandResult ConsoleLogout(const Core::StringVector &params);
		Console::CommandResult ConsoleCreateSession(const Core::StringVector &params);
		Console::CommandResult ConsoleCloseSession(const Core::StringVector &params);
		Console::CommandResult ConsoleListSessions(const Core::StringVector &params);
		Console::CommandResult ConsoleSendMessage(const Core::StringVector &params);
		Console::CommandResult ConsoleListContacts(const Core::StringVector &params);
		Console::CommandResult ConsolePublishPresence(const Core::StringVector &params);
		Console::CommandResult ConsoleSendFriendRequest(const Core::StringVector &params);
		Console::CommandResult ConsoleRemoveFriend(const Core::StringVector &params);
		Console::CommandResult ConsoleListFriendRequests(const Core::StringVector &params);
		Console::CommandResult ConsoleAcceptFriendRequest(const Core::StringVector &params);
		Console::CommandResult ConsoleDenyFriendRequest(const Core::StringVector &params);

	protected:
		static CommunicationManager* GetInstance(); // for python callbacks

		void InitializePythonCommunication();
		void UninitializePythonCommunication();
		void RegisterConsoleCommands();
		void RegisterEvents();

		void RemoveIMSession(const IMSession* s);
		ContactPtr GetContact(std::string id);
		IMSessionPtr CreateIMSession(ContactPtr contact, ContactPtr originator);
		void RequestPresenceStatuses();
		void CallPythonCommunicationObject(const std::string &method_name, const std::string &arg) const;
		void CallPythonCommunicationObject(const std::string &method_name) const;

		void StartFarsight2();
		void StopFarsight2();
		Farsight2Thread* farsight2_thread_;
		DBusDaemonThread* dbus_daemon_thread_;


		void StartDBusService();
    public:

        //! public because: need to access these from CommunicationSettings
		Foundation::ScriptObject* CallPythonCommunicationObjectAndGetReturnValue(const std::string &method_name, const std::string &arg) const;

		//! public because: need to access these from CommunicationSettings
		Foundation::ScriptObject* CallPythonCommunicationObjectAndGetReturnValue(const std::string &method_name) const;

    protected:

		//! pointer for CommuniocationManager instance
		//! Used by GetInstance member function
		static CommunicationManager* instance_;

		//! true if initialization is completed successfully
		bool initialized_; 

		//! true when connection is open to IM server
		//! \todo replace this with "Connection" or "ConnectionStatus" object
		bool connected_; 

		//! pointer to viewer framework object
		Foundation::Framework* framework_;

		//! python module object that contains from COMMUNICATION_PYTHON_MODULE path
		Foundation::ScriptObject* communication_py_script_; 

		//! python object of COMMUNICATION_PYTHON_CLASS class
		Foundation::ScriptObject* python_communication_object_; 

		//! Viewer's event manager
		Foundation::EventManagerPtr event_manager_;

		//! Event category for events send by CommmunicationModule 
		Core::event_category_id_t comm_event_category_; // \todo could be static 

		//! Placeholder for all open IM session
		IMSessionListPtr im_sessions_;

		//! Placeholder for all contacts of current users
		ContactList contact_list_;

		//! Presence status of current user
		PresenceStatusPtr presence_status_;

		//! Available options for presence status
		std::vector<std::string> presence_status_options_;

		//! Placeholder for active friend requests
		FriendRequestListPtr friend_requests_;

		//! Current user
		ContactPtr user_;

		//! Current protocl
		//! \todo We should refactore comm module so that multiple sessions for different IM serveces is possible
		std::string protocol_;

		// python event handlers
		// \todo: could these be non-static member functions?
		// \todo: could we handle memory allocation in some another way with these function parameters?
		static void PyCallbackTest(char *);
		static void PyCallbackConnected(char*);
		static void PyCallbackConnecting(char*);
		static void PyCallbackDisconnected(char*);
		static void PyCallbackChannelOpened(char*);
		static void PyCallbackChannelClosed(char*);
		static void PyCallbackMessagReceived(char*);
		static void PycallbackContactReceived(char* contact);
		static void PyCallbackPresenceStatusChanged(char* id);
		static void PyCallbackMessageSent(char* id);
		static void PyCallbackFriendRequest(char* id);
		static void PyCallbackContactRemoved(char* id);
		static void PyCallbackContactAdded(char* id);
		static void PyCallbackFriendRequestLocalPending(char* id);
		static void PyCallbackFriendRequestRemotePending(char* id);
		static void PyCallbackFriendAdded(char* id);
		static void PyCallbackPresenceStatusTypes(char* id);
		static void PyCallbackAccountCreationSucceeded(char* id);
        static void PyCallbackAccountCreationFailed(char* id);
	};


} // end of namespace Communication



#endif // incl_CommunicationManager_h

