#ifndef _LBTREE_HXX
#define _LBTREE_HXX

#ifdef WIN32
    #define __export
#endif


class Link
{
private:

  Link  *        prev;
  Link  *        next;
  Link  *        parent;

  char  *        string;
  unsigned long  litemdata;
  int            expanded;
  void *         pdata;
  void *         pExternItemData;

public:
  unsigned int  strExtent;
  int           hasChild;
  Link  (unsigned long ,Link * ,Link * ,Link * ,char *,void *);
  ~Link ();
  int           ToggleMode();
  unsigned long Item()                { return litemdata;}
  char*         String()              { return string;}
  int           SetNewString(char *p);

  void          SetString(char * p)   { if (string) delete string;string=p;}
  int           ParentLevel(Link * parent) {int ret=0;
                   for (Link *l=this;l->Parent();l=l->Parent()) {
                   ret++; if (l->Parent()==parent) return ret; } return 0;}
  Link*         Parent()              { return parent;}
  Link*         Next()                { return next;}
  Link*         Prev()                { return prev;}
  int           IsExpanded()          { return expanded;}
  int           IsParent()            { return hasChild;}
  void          SetData(void *p)      { if (pdata) delete pdata; pdata=p;}
  void  *       GetData(void )        { return pdata;}
  void          SetInt(int pos, int value){ * ( ((int *)pdata) +pos)=value;}
  int           Level(); 
  int           IsVisible();
  void *        GetItemData()         {return pExternItemData;}
  void *        SetItemData(void *p ) {void * old= pExternItemData; pExternItemData=p; return old;}
};

class LinkNonResolved : public Link
{
public:
  unsigned long  lparentitemdata;
  int      bDeleted;
  LinkNonResolved (unsigned long ul,Link * prev,char * p,unsigned long ulParent,void *lpItemData);
};

class Linked_List
{
protected:

  Link*           first;
  Link*           last;
  Link*           current;
  Link*           lastparent;
  LinkNonResolved * firstNonResolved;
  unsigned long   count;

  // Debug Emb 26/6/95 : performance measurement data
  unsigned long lFindDebugEmbCount;   // nb of calls to Find()
  unsigned long lNextDebugEmbCount;   // nb of calls to Next() from Find()
  unsigned long lFind2DebugEmbCount;  // nb of calls to Find(2params)
  unsigned long lNext2DebugEmbCount;  // nb of calls to Next() from Find(2params)
  unsigned long lNextADebugEmbCount;  // nb of calls to Next() from any place

  // Emb 28/6/95 : Find() performance improvement
  Link*         lastfound;            // found by the last Find()
  // Emb 23/8/95 : Find() performance improvement - second helping
  Link*         beforelastfound;      // before the last found

public:

  Linked_List();
  ~Linked_List ();

  void   Clear();
  Link * Add(unsigned long ,unsigned long ,char *, void * ,unsigned long ,unsigned long );

  void   Delete(Link *);
  void   Delete(unsigned long);
  void   ResolveLink(unsigned long ID);

  Link * First()  {current =first ; return first;}
  Link * Last()   {current =last  ; return last ;}
  Link * Current(){return current;}

  Link * Find(unsigned long ID);
  Link * Find(unsigned long ID,int bSearchNonResolved);

  int    ToggleMode(unsigned long ID);
  int    ToggleMode(Link *l);

  Link * Prev();
  Link * Next();  
  unsigned long  Count() {return count;}
  int    bSort;

  // Debug Emb 26/6/95 : performance measurement data
  void  ResetDebugEmbCounts();
};

#endif //_LBTREE_HXX
