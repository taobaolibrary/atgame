#pragma once

//////////////////////////////////////////////////////////////////////////
// this profile is part of bullet physics engine.
//////////////////////////////////////////////////////////////////////////

///A node in the Profile Hierarchy Tree
class   CProfileNode {

public:
    CProfileNode( const char * name, CProfileNode * parent );
    ~CProfileNode( void );

    CProfileNode * Get_Sub_Node( const char * name );

    CProfileNode * Get_Parent( void )       { return Parent; }
    CProfileNode * Get_Sibling( void )      { return Sibling; }
    CProfileNode * Get_Child( void )            { return Child; }

    void                CleanupMemory();
    void                Reset( void );
    void                Call( void );
    bool                Return( void );

    const char *    Get_Name( void )                { return Name; }
    int             Get_Total_Calls( void )     { return TotalCalls; }
    float               Get_Total_Time( void )      { return TotalTime; }
    void*           GetUserPointer() const {return m_userPtr;}
    void            SetUserPointer(void* ptr) { m_userPtr = ptr;}
protected:

    const char *    Name;
    int             TotalCalls;
    float           TotalTime;
    uint64          StartTime;
    int             RecursionCounter;

    CProfileNode *  Parent;
    CProfileNode *  Child;
    CProfileNode *  Sibling;
    void*   m_userPtr;
};


///An iterator to navigate through the tree
class CProfileIterator
{
public:
    // Access all the children of the current parent
    void                First(void);
    void                Next(void);
    bool                Is_Done(void);
    bool                Is_Root(void) { return (CurrentParent->Get_Parent() == 0); }

    void                Enter_Child( int index );       // Make the given child the new parent
    void                Enter_Largest_Child( void );    // Make the largest child the new parent
    void                Enter_Parent( void );           // Make the current parent's parent the new parent

    // Access the current child
    const char *        Get_Current_Name( void )            { return CurrentChild->Get_Name(); }
    int                 Get_Current_Total_Calls( void ) { return CurrentChild->Get_Total_Calls(); }
    float               Get_Current_Total_Time( void )  { return CurrentChild->Get_Total_Time(); }

    void*               Get_Current_UserPointer( void )         { return CurrentChild->GetUserPointer(); }
    void                Set_Current_UserPointer(void* ptr) {CurrentChild->SetUserPointer(ptr);}
    // Access the current parent
    const char *        Get_Current_Parent_Name( void )         { return CurrentParent->Get_Name(); }
    int                 Get_Current_Parent_Total_Calls( void )  { return CurrentParent->Get_Total_Calls(); }
    float               Get_Current_Parent_Total_Time( void )   { return CurrentParent->Get_Total_Time(); }



protected:

    CProfileNode *  CurrentParent;
    CProfileNode *  CurrentChild;


    CProfileIterator( CProfileNode * start );
    friend  class       CProfileManager;
};


///The Manager for the Profile system
class   CProfileManager {
public:
    static  void                Start_Profile( const char * name );
    static  void                Stop_Profile( void );

    static  void                CleanupMemory(void)
    {
        Root.CleanupMemory();
    }

    static  void                Reset( void );
    static  void                Increment_Frame_Counter( void );
    static  int                 Get_Frame_Count_Since_Reset( void )     { return FrameCounter; }
    static  float               Get_Time_Since_Reset( void );

    static  CProfileIterator *  Get_Iterator( void )    
    { 

        return new CProfileIterator( &Root ); 
    }
    static  void                Release_Iterator( CProfileIterator * iterator ) { delete ( iterator); }

    static void                 dumpRecursive(CProfileIterator* profileIterator, int spacing);

    static void                 dumpAll();

private:
    static  CProfileNode            Root;
    static  CProfileNode *          CurrentNode;
    static  int                     FrameCounter;
    static  uint64                  ResetTime;
};


///ProfileSampleClass is a simple way to profile a function's scope
///Use the BT_PROFILE macro at the start of scope to time
class   CProfileSample {
public:
    CProfileSample( const char * name )
    { 
        CProfileManager::Start_Profile( name ); 
    }

    ~CProfileSample( void )                 
    { 
        CProfileManager::Stop_Profile(); 
    }
};


#define ATG_PROFILE( name )         CProfileSample __profile( name )