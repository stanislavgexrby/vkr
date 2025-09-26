unit ErrorUnit;

interface
resourcestring
  CantFindName = 'Can''t find Identifier>%s< in Dictionaries';
  CantFindChar = 'Waiting for char ''%s''';
  CantAddSemantic = 'Can''t add Contex Symbol>%s<:becouse name Already Exist';
  CantAddMacro = 'Can''t add Auxiliary Notions>%s<:becouse name Already Exist';
  CantAddNonTerminal = 'Can''t add NonTerminal>%s<:becouse name Already Exist';
  CantOpenAllMacro = 'Can''t open all Auxiliary Notions becouse >%s< has recursion definition';

Procedure ERROR(const msg:string);
Procedure WARNING(const msg:string);

implementation
uses Dialogs;

Procedure ERROR(const msg:string);
begin
  MessageDlg(msg,mtError,[mbOK],0);
end;
Procedure WARNING(const msg:string);
begin
  MessageDlg(msg,mtWarning,[mbOK],0);
end;
end.
