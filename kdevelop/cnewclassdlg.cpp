/***************************************************************************
                     cnewclassdlg.cpp - 
                             -------------------                                         

    begin                : 4 Oct 1998                                        
    copyright            : (C) 1998 by Sandy Meier                         
    email                : smeier@rz.uni-potsdam.de                                     
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/

#include <kquickhelp.h>
#include <qlayout.h>
#include <klocale.h>
#define LAYOUT_BORDER (10)
#include "cnewclassdlg.h"
#include "cproject.h"

// SETUP DIALOG
CNewClassDlg::CNewClassDlg( QWidget *parent, const char *name,CProject* prj )
    : QDialog( parent, name,TRUE )
{
  prj_info = prj;
  header_modified =false;
  impl_modified = false;
  setCaption(i18n("Class Generator" ));
////
  QGridLayout * main_layout = new QGridLayout( this, 4, 2, LAYOUT_BORDER );

  classname_group = new QButtonGroup( this, "classname_group" );
  classname_group->setTitle(i18n("Classname") );
  main_layout->addWidget( classname_group, 0, 0 );

  QBoxLayout * layout = new QVBoxLayout( classname_group, LAYOUT_BORDER );
  layout->addSpacing( LAYOUT_BORDER );
  classname_edit = new QLineEdit( classname_group, "classname_edit" );
  classname_edit->setMinimumSize( classname_edit->sizeHint() );
  layout->addWidget( classname_edit );
	KQuickHelp::add(classname_group,
	KQuickHelp::add(classname_edit, i18n("Insert your new classname here.\n"
	                                    "Please try to keep some standards\n"
	                                    "like using an upper `K` as the first\n"
	                                    "letter for your new class, if it is\n"
	                                    "a class for a KDE-project.")));


  baseclass_group = new QButtonGroup( this, "baseclass_group" );
  baseclass_group->setTitle(i18n( "Baseclass") );
  //  baseclass_group->setAlignment( 1 );
  main_layout->addWidget( baseclass_group, 0, 1 );

  layout = new QVBoxLayout( baseclass_group, LAYOUT_BORDER );
  layout->addSpacing( LAYOUT_BORDER );
  baseclass_edit = new QLineEdit( baseclass_group, "baseclass_edit" );
  baseclass_edit->setMinimumSize( baseclass_edit->sizeHint() );
  layout->addWidget( baseclass_edit );
  KQuickHelp::add(baseclass_edit, i18n("Insert the baseclass from which your new class\n"
                                    "will be derived from. If you have checked\n"
                                    "`generate a QWidget-Childclass`, the new class will\n"
                                    "be derived from QWidget. If no baseclass is named, the\n"
                                    "new class will not have a parent class."));

  files_group = new QButtonGroup( this, "files_group" );
  files_group->setTitle( i18n("Files") );
  main_layout->addWidget( files_group, 1, 0 );
  //  files_group->setAlignment( 1 );


  layout = new QVBoxLayout( files_group, LAYOUT_BORDER );
  layout->addSpacing( LAYOUT_BORDER );
  header_label = new QLabel( files_group, "header_label" );
  header_label->setText( i18n("Header") );
  header_label->setMinimumSize( header_label->sizeHint() );
  layout->addWidget( header_label );

  header_edit = new QLineEdit( files_group, "header_edit" );
  header_edit->setMinimumSize( header_edit->sizeHint() );
  layout->addWidget( header_edit );

  KQuickHelp::add(header_label,
  KQuickHelp::add(header_edit, i18n("Insert your header filename here.\n"
                                    "It is automatically inserted while\n"
                                    "you select the classname, but you can\n"
                                    "still edit it afterwards.")));

  impl_label = new QLabel( files_group, "impl_label" );
  impl_label->setText( i18n("Impl.") );
  impl_label->setMinimumSize( impl_label->sizeHint() );
  layout->addWidget( impl_label );

  impl_edit = new QLineEdit( files_group, "impl_edit" );
  impl_edit->setMinimumSize( impl_edit->sizeHint() );
  layout->addWidget( impl_edit );

  KQuickHelp::add(impl_label,
  KQuickHelp::add(impl_edit, i18n("Insert your implementation filename here.\n"
                                  "It is automatically inserted while\n"
                                  "you select the classname, but you can\n"
                                  "still edit it afterwards.")));

  doc_group = new QButtonGroup( this, "doc_group" );
  doc_group->setTitle( i18n("Documentation") );
  main_layout->addWidget( doc_group, 2, 0 );


  layout = new QVBoxLayout( doc_group, LAYOUT_BORDER );
  layout->addSpacing( LAYOUT_BORDER );
  doc_edit = new QMultiLineEdit( doc_group, "doc_edit" );
  doc_edit->setMinimumSize( doc_edit->sizeHint() );
  layout->addWidget( doc_edit );
  KQuickHelp::add(doc_group,
  KQuickHelp::add(doc_edit, i18n("Insert a short description for your new class\n"
                                "here for documenation purposes. This will be used\n"
                                "for the API-documentation you can create or update\n"
                                "after the class is created.")));
	
  inher_group = new QButtonGroup( this, "inher_group" );
  inher_group->setTitle( i18n("Inheritance") );
  main_layout->addWidget( inher_group, 1, 1 );

  layout = new QVBoxLayout( inher_group, LAYOUT_BORDER );
  layout->addSpacing( LAYOUT_BORDER );
  public_button = new QRadioButton( inher_group, "public_button" );
  public_button->setText(i18n("public"));
  public_button->setChecked(true);
  public_button->setMinimumSize( public_button->sizeHint() );
  layout->addWidget( public_button );

  protected_button = new QRadioButton( inher_group, "protected_button" );
  protected_button->setText(i18n("protected"));
  protected_button->setMinimumSize( protected_button->sizeHint() );
  layout->addWidget( protected_button );

  private_button = new QRadioButton( inher_group, "private_button" );
  private_button->setText(i18n("private"));
  private_button->setMinimumSize( private_button->sizeHint() );
  layout->addWidget( private_button );
  layout->addStretch( 1 );

  add_group = new QButtonGroup( this, "add_group" );
  add_group->setTitle(i18n("Additional Options") );
  main_layout->addWidget( add_group, 2, 1 );

  layout = new QVBoxLayout( add_group, LAYOUT_BORDER );
  layout->addSpacing( LAYOUT_BORDER );
  template_check = new QCheckBox( add_group, "template_check" );
  template_check->setText( i18n("use header/cpp-templates") );
  template_check->setChecked(true);
  template_check->setMinimumSize( template_check->sizeHint() );
  layout->addWidget( template_check );

  qwidget_check = new QCheckBox( add_group, "qwidget_check" );
  qwidget_check->setText( i18n("generate a QWidget-Childclass") );
  qwidget_check->setMinimumSize( qwidget_check->sizeHint() );
  layout->addWidget( qwidget_check );
  layout->addStretch( 1 );

  add_group->insert( template_check );
  add_group->insert( qwidget_check );
  KQuickHelp::add(add_group,
  KQuickHelp::add(template_check,
  KQuickHelp::add(qwidget_check, i18n("This offers additional options to your new class.\n"
                                    "If you check `use header/cpp-templates`, your name and\n"
                                    "email adress will be added into the new generated files\n"
                                    "If you check `generate a QWidget-Childclass`, the new class\n"
                                    "will use QWidget as the baseclass. This is also useful for\n"
                                    "using baseclasses derived from QWidget, e.g. QDialog."))));


  inher_group->insert( public_button );
  inher_group->insert( protected_button );
  inher_group->insert( private_button );

  KQuickHelp::add(inher_group,
  KQuickHelp::add(protected_button,
  KQuickHelp::add(public_button,
  KQuickHelp::add(private_button, i18n("Check here your inheritance from the chosen baseclass.\n"
                                      "You can derive from a baseclass as public,protected\n"
                                      "or private.")))));


  layout = new QHBoxLayout;
  main_layout->addLayout( layout, 3, 1 );

  cancel_button = new QPushButton( this, "cancel_button" );
  cancel_button->setText(i18n("Cancel"));
  cancel_button->setFixedSize( cancel_button->sizeHint() );

  ok_button = new QPushButton( this, "ok_button" );
  ok_button->setText(i18n("OK"));
  ok_button->setDefault( TRUE );
  ok_button->setFixedSize( cancel_button->sizeHint() );
  layout->addStretch( 1 );
  layout->addWidget( ok_button );
  layout->addWidget( cancel_button );


  main_layout->setColStretch( 0, 1 );
  main_layout->setColStretch( 1, 0 );
  main_layout->setRowStretch( 0, 0 );
  main_layout->setRowStretch( 1, 0 );
  main_layout->setRowStretch( 2, 1 );
  main_layout->setRowStretch( 3, 0 );

  main_layout->activate();
	adjustSize();

////
  connect(classname_edit,SIGNAL(textChanged(const char*)),SLOT(slotClassEditChanged(const char*)));
  connect(header_edit,SIGNAL(textChanged(const char*)),SLOT(slotHeaderEditChanged(const char*)));
  connect(impl_edit,SIGNAL(textChanged(const char*)),SLOT(slotImplEditChanged(const char*)));
  connect(cancel_button,SIGNAL(clicked()),SLOT(reject()));
  connect(ok_button,SIGNAL(clicked()),SLOT(ok()));
  
  //setOkButton(i18n("OK"));
  // setCancelButton(i18n("Cancel"));
  //connect( this, SIGNAL(applyButtonPressed()), SLOT(ok()) );
}

void CNewClassDlg::ok(){
  QString text = classname_edit->text(); 
  if (text.isEmpty() ){
    KMsgBox::message(this,i18n("Error..."),i18n("You must enter a classname!"),KMsgBox::EXCLAMATION);
    return;
   }
  text = header_edit->text(); 
  if (text.isEmpty() ){
    KMsgBox::message(this,i18n("Error..."),i18n("You must enter a name for the header-file!"),KMsgBox::EXCLAMATION);
    return;
   }
  text = impl_edit->text(); 
  if (text.isEmpty() ){
    KMsgBox::message(this,i18n("Error..."),i18n("You must enter a name for the implementation-file!")
		     ,KMsgBox::EXCLAMATION);
    return;
  }
  // text = baseclass_edit->text(); 
//   if (text.isEmpty() && qwidget_check->isChecked()){
//     KMsgBox::message(this,i18n("Error..."),i18n("You must enter a name for the baseclass!")
// 		     ,KMsgBox::EXCLAMATION);
//     return;
//   }

 
  QString headername = prj_info->getProjectDir() + prj_info->getSubDir() + header_edit->text();
  QString implname = prj_info->getProjectDir() + prj_info->getSubDir() + impl_edit->text();
  QString headerfile = header_edit->text();
  QString classname = classname_edit->text();
  QString basename = baseclass_edit->text();
// added by Alex Kern, Alexander.Kern@saarsoft.de
//
  int slash = headername.findRev('/', -1, false);
  if(slash != -1)
    headerfile = headername.right(headername.length()-slash-1);

  if (QFileInfo(headername).exists() || QFileInfo(implname).exists()) {
      KMsgBox::message(this, i18n("Error..."),i18n("Sorry, but KDevelop is not able to add classes "
                                                   "to existing header or implementation files."),
                       KMsgBox::EXCLAMATION);
      return;
  }
 
  if(basename.isEmpty() && qwidget_check->isChecked()){
    basename = "QWidget";
  }
  QString doc = doc_edit->text();


  CGenerateNewFile generator;
  // generate the sourcecode


  if(template_check->isChecked()){
    headername = generator.genHeaderFile(headername,prj_info);
    implname = generator.genCPPFile(implname,prj_info);
  }
  else{
    QFile file(headername);
    file.open(IO_ReadWrite);
    file.close();
    file.setName(implname);
    file.open(IO_ReadWrite);
    file.close();
  }


// added by Alex Kern, Alexander.Kern@saarsoft.de
//
//  create implementationfile first, use headerfile like "header.h" before
//  change them to "HEADER_H"

  // modify the implementation
  QStrList list;
  QFile file(implname);
  QTextStream stream(&file);
  QString str;

  if(file.open(IO_ReadOnly)){ //
    while(!stream.eof()){
      list.append(stream.readLine());
    }
  }
  file.close();

  if(file.open(IO_WriteOnly)){
    for(str = list.first();str != 0;str = list.next()){
      stream << str << "\n";
    }
    stream << "\n#include \"" + QString(headerfile) + "\"\n\n" ;
    stream << classname + "::" + classname +"(" ;
    if (qwidget_check->isChecked()){
      stream << "QWidget *parent, const char *name ) : ";
      stream << basename + "(parent,name) {\n";
    }
    else{
      stream << "){\n";
    }
    stream << "}\n";
    stream << classname + "::~" + classname +"(){\n";
    stream << "}\n";
  }
  file.close();

  // modify the header
  list.clear();
  file.setName(headername);

  if(file.open(IO_ReadOnly)){ //
    while(!stream.eof()){
      list.append(stream.readLine());
    }
  }
  file.close();

  if(file.open(IO_WriteOnly)){
    for(str = list.first();str != 0;str = list.next()){
      stream << str << "\n";
    }
    headerfile.replace(QRegExp("\\."),"_");
    stream << "\n#ifndef " + headerfile.upper() + "\n";
    stream << "#define "+ headerfile.upper() + "\n\n";
    if (qwidget_check->isChecked() && basename != "QWidget"){
      stream << "#include <qwidget.h>\n";
    }
    if(!basename.isEmpty()){
      stream << "#include <" + basename.lower() + ".h>\n";
    }
    stream << "\n/**" + doc + "\n";
    stream << "  *@author "+ prj_info->getAuthor() + "\n";
    stream << "  */\n\n";
    stream << "class " + classname;
    if(!basename.isEmpty()){
      stream << " : ";
      if(public_button->isChecked()){
	stream << "public ";
      }
      if(protected_button->isChecked()){
	stream << "protected ";
      }
      if(private_button->isChecked()){
	stream << "private ";
      }
      stream << basename + " ";
    }
    stream << " {\n";
    if(qwidget_check->isChecked()){
      stream << "   Q_OBJECT\n";
    }
    stream << "public: \n";
    stream << "\t" + classname+"(";
    if (qwidget_check->isChecked()){
      stream << "QWidget *parent=0, const char *name=0";
    }
    stream << ");\n";
    stream << "\t~" + classname +"();\n";
    stream << "};\n\n#endif\n";
  }
  file.close();

  m_header_file = headername;
  m_impl_file = implname;
  accept();
}
QString CNewClassDlg::getHeaderFile(){
  return m_header_file;
}
QString CNewClassDlg::getImplFile(){
  return m_impl_file;
}

void CNewClassDlg::slotClassEditChanged(const char* text){
  QString str = text;
  if(!header_modified){
    header_edit->setText(str.lower() + ".h");
  }
  if(!impl_modified){
    impl_edit->setText(str.lower() +".cpp");
  }
  
}
void CNewClassDlg::slotHeaderEditChanged(const char*){
  if(header_edit->hasFocus()){
    header_modified = true;
  }
}
void CNewClassDlg::slotImplEditChanged(const char*){
  if(impl_edit->hasFocus()){
    impl_modified = true;
  }
}




