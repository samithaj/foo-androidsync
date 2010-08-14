#include "stdafx.h"
#include "resource.h"
#include <playlist.h>

// = Declarations =

void androidsync_do_sync_pl( t_size );
void androidsync_do_sync();

DECLARE_COMPONENT_VERSION(
   "Android Sync Component",
   "0.8.10",
   "Select the playlists you would like to synchronize from the \"Synchronize "
   "Playlists\" preference panel and then select \"Android Sync\" -> \""
   "Synchronize Playlists\" to perform synchronization."
);

// This will prevent users from renaming your component around (important for 
// proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME( "foo_androidsync.dll" );

static const GUID g_mainmenu_group_id = {
   // {ACE8A3BE-8E51-4FBA-BB22-4560643BC346}
   0xace8a3be, 0x8e51, 0x4fba, 
   { 0xbb, 0x22, 0x45, 0x60, 0x64, 0x3b, 0xc3, 0x46 }
};

// These GUIDs identify the variables within our component's configuration file.
static const GUID guid_cfg_selectedplaylists = {
   // {C1D68A14-464C-4B44-8DDE-A1723AFD881F}
   0xc1d68a14, 0x464c, 0x4b44, 
   { 0x8d, 0xde, 0xa1, 0x72, 0x3a, 0xfd, 0x88, 0x1f } 
};

static const GUID guid_cfg_targetpath = {
   // {A0EF91CA-E192-4F5A-9DA3-0B5F300E26C8}
   0xa0ef91ca, 0xe192, 0x4f5a, { 0x9d, 0xa3, 0xb, 0x5f, 0x30, 0xe, 0x26, 0xc8 }
};

static cfg_objList<pfc::string8> cfg_selectedplaylists( guid_cfg_selectedplaylists );
static cfg_string cfg_targetpath( guid_cfg_targetpath, "" );



/* = Main Menu Additions = */

static mainmenu_group_popup_factory g_mainmenu_group( 
   g_mainmenu_group_id, 
   mainmenu_groups::file, 
   mainmenu_commands::sort_priority_dontcare, 
   "Android Sync"
);

class mainmenu_commands_androidsync : public mainmenu_commands {
public:
	enum {
		cmd_sync = 0,
		cmd_total
	};

	t_uint32 get_command_count() {
		return cmd_total;
	}

	GUID get_command( t_uint32 p_index ) {
		static const GUID guid_sync = {
         // {F24A7301-5B86-420B-A1E8-B1EE83CEF4DD}
         0xf24a7301, 0x5b86, 0x420b, 
         { 0xa1, 0xe8, 0xb1, 0xee, 0x83, 0xce, 0xf4, 0xdd }

      };

		switch(p_index) {
			case cmd_sync: 
            return guid_sync;

			default: 
            // This should never happen unless somebody called us with invalid 
            // parameters; bail.
            uBugCheck();
		}
	}

	void get_name( t_uint32 p_index, pfc::string_base &p_out ) {
		switch( p_index ) {
			case cmd_sync: 
            p_out = "Synchronize Playlists";
            break;

			default: 
            // This should never happen unless somebody called us with invalid 
            // parameters; bail.
            uBugCheck();
		}
	}

	bool get_description( t_uint32 p_index,pfc::string_base &p_out ) {
		switch( p_index ) {
			case cmd_sync:
            p_out = "Synchronize the playlists selected in preferences to "
               "the connected android device";
            return true;

			default: 
            // This should never happen unless somebody called us with invalid 
            // parameters; bail.
            uBugCheck();
		}
	}

	GUID get_parent() {
		return g_mainmenu_group_id;
	}

	void execute( t_uint32 p_index, service_ptr_t<service_base> p_callback ) {
		switch( p_index ) {
			case cmd_sync:
				androidsync_do_sync();
				break;

			default:
            // This should never happen unless somebody called us with invalid 
            // parameters; bail.
				uBugCheck();
		}
	}
};

static mainmenu_commands_factory_t<mainmenu_commands_androidsync> 
   g_mainmenu_commands_androidsync_factory;



/* = Preferences = */

class CAndroidSyncPrefs : public CDialogImpl<CAndroidSyncPrefs>, 
public preferences_page_instance {
public:
	// Constructor, invoked by preferences_page_impl helpers. Don't do Create()
   // in here, preferences_page_impl does this for us.
	CAndroidSyncPrefs( preferences_page_callback::ptr callback ) : 
      m_callback( callback ) {}

	// Note that we don't bother doing anything regarding destruction of our 
   // class. The host ensures that our dialog is destroyed first, then the 
   // last reference to our preferences_page_instance object is released, 
   // causing our object to be deleted.

	// Dialog Resource ID
	enum { IDD = IDD_ANDROIDSYNCPREFS };

   // preferences_page_instance methods (not all of them - get_wnd() is 
   // supplied by preferences_page_impl helpers).
	t_uint32 get_state() {
	   t_uint32 state = preferences_state::resettable;
	
      if( has_changed() ) {
         state |= preferences_state::changed;
      }

	   return state;
   }

   void apply() {
	   // cfg_selectedplaylists = GetDlgItemInt(IDC_SELECTEDPLAYLISTS, NULL, FALSE);
	   // cfg_targetpath = GetDlgItemInt(IDC_TARGETPATH, NULL, FALSE);
      int item_count,
         i,
         item_index_iter,
         * item_indexes;
      pfc::string8 item_iter_8;
      LPTSTR targetpath_wide = new TCHAR[MAX_PATH];

      // Clear the existing list of selected playlists before we begin.
      cfg_selectedplaylists.remove_all();

      // Get the currently selected playlists from the listbox.
      item_count = SendDlgItemMessage(
         IDC_SELECTEDPLAYLISTS,
         LB_GETSELCOUNT,
         NULL, NULL
      );
      item_indexes = new int[item_count];
      SendDlgItemMessage(
         IDC_SELECTEDPLAYLISTS,
         LB_GETSELITEMS,
         (WPARAM)item_count,
         (LPARAM)item_indexes
      );

      // Save the currently selected list of playlists to the configuration
      // variable.
      for( i = 0 ; i < item_count ; i++ ) {
         item_index_iter = item_indexes[i];
      
         // XXX: This just seems to return blank strings/crashes/heartache.
         /* item_iter_wide = (LPCTSTR)SendDlgItemMessage(
            IDC_SELECTEDPLAYLISTS,
            LB_GETITEMDATA,
            (WPARAM)item_index_iter,K
            0
         ); */
      
         // This is kind of cheating since we're not pulling it from the listbox
         // itself, but it will probably work for the time being since we're 
         // pulling from the same source it gets the names from.
         static_api_ptr_t<playlist_manager>()->playlist_get_name( 
            item_index_iter, item_iter_8 
         );

         cfg_selectedplaylists.add_item( item_iter_8 );
      }
      delete item_indexes;

      // Save the current target path.
      GetDlgItemText( IDC_TARGETPATH, targetpath_wide, MAX_PATH );
      cfg_targetpath.set_string(
         pfc::stringcvt::string_utf8_from_wide( targetpath_wide )
      );

      // Our dialog content has not changed but the flags have - our currently 
      // shown values now match the settings so the apply button can be disabled.
	   on_change();
   }

   void reset() {
      SendDlgItemMessage(
         IDC_SELECTEDPLAYLISTS,
         LB_SETSEL,
         (WPARAM)FALSE,
         (LPARAM)-1
      );
      SetDlgItemText( IDC_TARGETPATH, _T( "" ) );

      on_change();
   }

	// WTL Message Map
	BEGIN_MSG_MAP( CAndroidSyncPrefs )
		MSG_WM_INITDIALOG( on_init_dialog )
		COMMAND_HANDLER_EX( IDC_SELECTEDPLAYLISTS, LBN_SELCHANGE, on_sel_change )
		COMMAND_HANDLER_EX( IDC_TARGETPATH, EN_CHANGE, on_edit_change )
	END_MSG_MAP()

private:

   BOOL on_init_dialog(CWindow, LPARAM) {
      t_size plist_id_iter, // ID of the currently iterated playlist.
         cpare_id_iter; // ID of the string plist_iter is compared against.
      pfc::string8 cpare_iter,
         playlist_name;

      // Populate the list box with all available playlists.
      for( 
         plist_id_iter = 0;
         plist_id_iter < static_api_ptr_t<playlist_manager>()->get_playlist_count();
         plist_id_iter++
      ) {
   
         static_api_ptr_t<playlist_manager>()->playlist_get_name( 
            plist_id_iter, playlist_name 
         );

         // Modern Foobar plugin projects have to be unicode-enabled, which causes
         // all Windows API functions to require wide character inputs. However,
         // the playlist manager function above uses the proprietary PFC string 
         // class which is not wide. Therefore, the string must be converted using 
         // the stringcvt helper function. This information was found conveniently 
         // in the cellar of the Foobar documentation, with the aid of a 
         // flashlight, in the bottom of a locked filing cabinet stuck in a disused
         // lavatory with a sign on the door saying, "Beware of the Leopard".
         SendDlgItemMessage(
            IDC_SELECTEDPLAYLISTS,
            LB_ADDSTRING,
            (WPARAM)0,
            (LPARAM)((LPCTSTR)pfc::stringcvt::string_wide_from_utf8( playlist_name ))
         );

         // Select user-selected playlists in the list box.
         for( 
            cpare_id_iter = 0;
            cpare_id_iter < cfg_selectedplaylists.get_count();
            cpare_id_iter++
         ) {
            cpare_iter = cfg_selectedplaylists.get_item( cpare_id_iter );

            if( 0 == pfc::comparator_strcmp::compare( cpare_iter, playlist_name ) ) {
               SendDlgItemMessage(
                  IDC_SELECTEDPLAYLISTS,
                  LB_SETSEL,
                  (WPARAM)TRUE,
                  (LPARAM)plist_id_iter
               );
            }
         }
      }

      // Set the target path control to the user-selected value.
      SetDlgItemText(
         IDC_TARGETPATH, 
         pfc::stringcvt::string_wide_from_utf8( cfg_targetpath )
      );
   
	   return FALSE;
   }

   void on_edit_change(UINT, int, CWindow) {
	   on_change();
   }

   void on_sel_change(UINT, int, CWindow) {
      on_change();
   }

   void on_change() {
	   // Tell the host that our state has changed to enable/disable the apply 
      // button appropriately.
	   m_callback->on_state_changed();
   }

   // Returns whether our dialog content is different from the current 
   // configuration (whether the apply button should be enabled or not).
   bool has_changed() {
	   /* LPCTSTR targetpath_text;
      bool changed = false;
      int item_count,
         i,
         item_index_iter,
         * item_indexes;
      pfc::string8 item_iter_8;

      // Get the currently selected playlists from the listbox.
      item_count = SendDlgItemMessage(
         IDC_SELECTEDPLAYLISTS,
         LB_GETSELCOUNT,
         NULL, NULL
      );
      item_indexes = new int[item_count];
      SendDlgItemMessage(
         IDC_SELECTEDPLAYLISTS,
         LB_GETSELITEMS,
         (WPARAM)item_count,
         (LPARAM)item_indexes
      ); */

      // See if the playlist selections have changed.
      /* for( i = 0 ; i < item_count ; i++ ) {
         item_index_iter = item_indexes[i];

         // This is kind of cheating since we're not pulling it from the listbox
         // itself, but it will probably work for the time being since we're 
         // pulling from the same source it gets the names from.
         static_api_ptr_t<playlist_manager>()->playlist_get_name( 
            item_index_iter, item_iter_8 
         );

         if( 0 != cfg_selectedplaylists.find_item( item_iter_8 ) ) {
         
         }
      }
      delete item_indexes; */

      // See if the target path changed.
      /* GetDlgItemText( IDC_SELECTEDPLAYLISTS, targetpath_text,  );
      if( 0 != pfc::comparator_strcmp::compare( targetpath_text, cfg_targetpath ) ) {
         changed = true;
      } */

      // return changed;
      // XXX
      return true;
   }
   
   const preferences_page_callback::ptr m_callback;
};

class preferences_page_androidsync : 
public preferences_page_impl<CAndroidSyncPrefs> {
public:
	const char * get_name() { return "Android Sync"; }

	GUID get_guid() {
		static const GUID guid = {
         // {77329F7B-F28A-404A-8A0B-5F50426F4211}
         0x77329f7b, 0xf28a, 0x404a, 
         { 0x8a, 0xb, 0x5f, 0x50, 0x42, 0x6f, 0x42, 0x11 }
      };

      return guid;
	}

	GUID get_parent_guid() { return guid_tools; }
};

static preferences_page_factory_t<preferences_page_androidsync>
   g_preferences_page_androidsync_factory;



/* = Synchronization = */

void androidsync_do_sync_pl( t_size playlist_id_in ) {
   pfc::list_t<metadb_handle_ptr> playlist_items; // List of plist items.
   t_size item_iter_id; // Index of the currently processing plist item.
   pfc::string_formatter cmd_files; // File list to pass to SHFileCopy.
   
   static_api_ptr_t<playlist_manager>()->playlist_get_all_items(
      playlist_id_in, playlist_items
   );
   
   for( 
      item_iter_id = 0;
      item_iter_id < playlist_items.get_count();
      item_iter_id++
   ) {
      cmd_files << 
         playlist_items.get_item( item_iter_id ).get_ptr()->get_path();
      cmd_files.add_char( '\0' );
   }
   cmd_files.add_char( '\0' );

   /* pfc::string8 tmp1;
   static_api_ptr_t<playlist_manager>()->playlist_get_name( playlist_id_in, tmp1 );
   popup_message::g_show( tmp1, "Blah" );
   popup_message::g_show( cmd_files, "Blah" ); */
}

void androidsync_do_sync() {

   t_size plist_id_iter, // Iterator for all playlist IDs.
       selected_id_iter; // Iterator for selected playlist IDs.
   pfc::string8 plist_iter,
      selected_iter;

   for( 
      plist_id_iter = 0;
      plist_id_iter < static_api_ptr_t<playlist_manager>()->get_playlist_count();
      plist_id_iter++
   ) {
      for(
         selected_id_iter = 0;
         selected_id_iter < cfg_selectedplaylists.get_count();
         selected_id_iter++
      ) {
         static_api_ptr_t<playlist_manager>()->playlist_get_name( 
            plist_id_iter, plist_iter
         );
         selected_iter = cfg_selectedplaylists.get_item( selected_id_iter );
         if( 
            0 == pfc::comparator_strcmp::compare( plist_iter, selected_iter )
         ) {
            // This playlist is on the list of selected playlists.
            androidsync_do_sync_pl( plist_id_iter );
            break;
         }
      }
   }
}