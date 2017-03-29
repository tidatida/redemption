/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2010-2013
   Author(s): Christophe Grosjean, Clément Moroldo
*/



#pragma once

#include "utils/log.hpp"

#ifndef Q_MOC_RUN
#include <stdio.h>
#include <openssl/ssl.h>
#include <iostream>
#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>

#include "utils/genfstat.hpp"
#include "core/front_api.hpp"
#include "mod/mod_api.hpp"
#include "mod/rdp/rdp_log.hpp"
#include "transport/socket_transport.hpp"
#include "keymaps/qt_scancode_keymap.hpp"
#include "core/client_info.hpp"
#include "mod/internal/replay_mod.hpp"
#include "configs/config.hpp"
#include "utils/bitmap.hpp"
#include "core/RDP/bitmapupdate.hpp"

#include "capture/flv_params_from_ini.hpp"
#include "capture/capture.hpp"
#include "core/RDP/MonitorLayoutPDU.hpp"


#include "Qt4/Qt.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic pop

#endif

#define REPLAY_PATH "/replay"
#define LOGINS_PATH "/config/logins.config"




struct DummyAuthentifier : public auth_api
{
public:
    virtual void set_auth_channel_target(const char *) {}
    virtual void set_auth_error_message(const char *) {}
    virtual void report(const char * , const char *) {}
    virtual void log4(bool , const char *, const char * = nullptr) {}
    virtual void disconnect_target() {}
};



class Front_Qt_API : public FrontAPI
{

public:
    RDPVerbose        verbose;
    ClientInfo        info;
    std::string       user_name;
    std::string       user_password;
    std::string       target_IP;
    int               port;
    std::string       local_IP;
    int               fps = 0;


    int                        nbTry;
    int                        retryDelay;
    int                        delta_time;
    mod_api                  * mod;
    std::unique_ptr<ReplayMod> replay_mod;
    bool                 is_recording;
    bool                 is_replaying;


    QImage::Format       imageFormatRGB;
    QImage::Format       imageFormatARGB;
    Qt_ScanCode_KeyMap   qtRDPKeymap;
    QPixmap            * cache;
    QPixmap            * cache_replay;
    SocketTransport    * socket;
    TimeSystem           timeSystem;
    DummyAuthentifier    authentifier;
    bool                 is_spanning;


    const std::string    MAIN_DIR;
    const std::string    REPLAY_DIR;
    const std::string    USER_CONF_LOG;


    Front_Qt_API( RDPVerbose verbose)
    : FrontAPI(false, false)
    , verbose(verbose)
    , info()
    , port(0)
    , local_IP("unknow_local_IP")
    , nbTry(3)
    , retryDelay(1000)
    , delta_time(1000000)
    , mod(nullptr)
    , replay_mod(nullptr)
    , is_recording(false)
    , is_replaying(false)
    , qtRDPKeymap()
    , cache(nullptr)
    , cache_replay(nullptr)
    , socket(nullptr)
    , is_spanning(false)
    , MAIN_DIR(std::string(MAIN_PATH))
    , REPLAY_DIR(MAIN_DIR + std::string(REPLAY_PATH))
    , USER_CONF_LOG(MAIN_DIR + std::string(LOGINS_PATH))
    {}

    virtual void send_to_channel( const CHANNELS::ChannelDef & , uint8_t const *
                                , std::size_t , std::size_t , int ) override {}

    // CONTROLLER
    virtual bool connexionReleased() = 0;
    virtual void closeFromScreen() = 0;
    virtual void RefreshPressed() = 0;
    virtual void CtrlAltDelPressed() = 0;
    virtual void CtrlAltDelReleased() = 0;
    virtual void disconnexionReleased() = 0;
    virtual void setMainScreenOnTopRelease() = 0;
    virtual void mousePressEvent(QMouseEvent *e, int screen_index) = 0;
    virtual void mouseReleaseEvent(QMouseEvent *e, int screen_index) = 0;
    virtual void keyPressEvent(QKeyEvent *e) = 0;
    virtual void keyReleaseEvent(QKeyEvent *e) = 0;
    virtual void wheelEvent(QWheelEvent *e) = 0;
    virtual bool eventFilter(QObject *obj, QEvent *e, int screen_index) = 0;
    virtual void disconnect(std::string const & txt) = 0;
    virtual void dropScreen() = 0;

    virtual void replay(std::string const & movie_path) = 0;
    virtual void load_replay_mod(std::string const & movie_name) = 0;
    virtual void delete_replay_mod() = 0;
    virtual void callback() = 0;

    virtual bool can_be_start_capture() override { return true; }
    virtual bool must_be_stop_capture() override { return true; }


    virtual void options() {
        LOG(LOG_WARNING, "No options window implemented yet.");
    };

    virtual mod_api * init_mod() = 0;

};



class Mod_Qt : public QObject
{

Q_OBJECT

public:
    Front_Qt_API              * _front;
    QSocketNotifier           * _sckRead;
    mod_api                   * _callback;
    SocketTransport           * _sck;
    int                         _client_sck;
    std::string error_message;

    QTimer timer;


    Mod_Qt(Front_Qt_API * front, QWidget * parent)
        : QObject(parent)
        , _front(front)
        , _sckRead(nullptr)
        , _callback(nullptr)
        , _sck(nullptr)
        , _client_sck(0)
        , timer(this)
    {}

    ~Mod_Qt() {
        this->disconnect();
    }


    void disconnect() {

        if (this->_sckRead != nullptr) {
            delete (this->_sckRead);
            this->_sckRead = nullptr;
        }

        if (this->_callback != nullptr) {
            TimeSystem timeobj;
            this->_callback->disconnect(timeobj.get_time().tv_sec);
            delete (this->_callback);
            this->_callback = nullptr;
            this->_front->mod = nullptr;
        }

        if (this->_sck != nullptr) {
            delete (this->_sck);
            this->_sck = nullptr;
            LOG(LOG_INFO, "Disconnected from [%s].", this->_front->target_IP.c_str());
        }
    }

    bool connect() {
        const char * name(this->_front->user_name.c_str());
        const char * targetIP(this->_front->target_IP.c_str());
        const std::string errorMsg("Cannot connect to [" + this->_front->target_IP +  "].");

        this->_client_sck = ip_connect(targetIP, this->_front->port, this->_front->nbTry, this->_front->retryDelay);

        if (this->_client_sck > 0) {
            try {

                this->_sck = new SocketTransport( name
                                                , this->_client_sck
                                                , targetIP
                                                , this->_front->port
                                                , to_verbose_flags(0)
                                                , &this->error_message
                                                );

                this->_front->socket = this->_sck;
                LOG(LOG_INFO, "Connected to [%s].", targetIP);

            } catch (const std::exception &) {
                std::string windowErrorMsg(errorMsg+" Socket error.");
                LOG(LOG_WARNING, "%s", windowErrorMsg.c_str());
                this->_front->disconnect("<font color='Red'>"+windowErrorMsg+"</font>");
                return false;
            }
        } else {
            std::string windowErrorMsg(errorMsg+" ip_connect error.");
            LOG(LOG_WARNING, "%s", windowErrorMsg.c_str());
            this->_front->disconnect("<font color='Red'>"+windowErrorMsg+"</font>");
            return false;
        }

        return true;
    }

    bool listen() {

        this->_callback = this->_front->init_mod();

        if (this->_callback !=  nullptr) {

            this->_sckRead = new QSocketNotifier(this->_client_sck, QSocketNotifier::Read, this);
            this->QObject::connect(this->_sckRead,   SIGNAL(activated(int)), this,  SLOT(call_draw_event()));

            this->QObject::connect(&(this->timer),   SIGNAL(timeout()), this,  SLOT(call_draw_event()));

            if (this->_callback) {
                if (this->_callback->get_event().set_state) {
                    struct timeval now = tvtime();
                    int time_to_wake = (this->_callback->get_event().trigger_time.tv_usec - now.tv_usec) / 1000
                    + (this->_callback->get_event().trigger_time.tv_sec - now.tv_sec) * 1000;


                    if (time_to_wake < 0) {
                        this->timer.stop();
                    } else {
                        this->timer.start( time_to_wake );
                    }

                }
            } else {
                const std::string errorMsg("Error: Mod Initialization failed.");
                std::string labelErrorMsg("<font color='Red'>"+errorMsg+"</font>");
                this->_front->dropScreen();
                this->_front->disconnect(labelErrorMsg);
                return false;
            }

        } else {
            const std::string errorMsg("Error: Mod Initialization failed.");
            std::string labelErrorMsg("<font color='Red'>"+errorMsg+"</font>");
            this->_front->dropScreen();
            this->_front->disconnect(labelErrorMsg);
            return false;
        }

        return true;
    }


public Q_SLOTS:
    void call_draw_event() {
        if (this->_front->mod) {

            this->_front->callback();

            if (this->_callback) {
                if (this->_callback->get_event().set_state) {
                    struct timeval now = tvtime();
                    int time_to_wake = ((this->_callback->get_event().trigger_time.tv_usec - now.tv_usec) / 1000)
                    + ((this->_callback->get_event().trigger_time.tv_sec - now.tv_sec) * 1000);

                    if (time_to_wake < 0) {
                        this->timer.stop();
                    } else {
                        this->timer.start( time_to_wake );
                    }
                } else {
                    this->timer.stop();
                }
            }
        }
    }



};







class Form_Qt : public QWidget
{

Q_OBJECT

public:
    enum : int {
        MAX_ACCOUNT_DATA = 10
    };

    Front_Qt_API       * _front;
    const int            _width;
    const int            _height;
    QFormLayout          _formLayout;
    QComboBox            _IPCombobox;
    int                  _accountNB;
    QLineEdit            _IPField;
    QLineEdit            _userNameField;
    QLineEdit            _PWDField;
    QLineEdit            _portField;
    QLabel               _IPLabel;
    QLabel               _userNameLabel;
    QLabel               _PWDLabel;
    QLabel               _portLabel;
    QLabel               _errorLabel;
    QCheckBox            _pwdCheckBox;
    QPushButton          _buttonConnexion;
    QPushButton          _buttonOptions;
    QPushButton          _buttonReplay;
    QCompleter         * _completer;
    struct AccountData {
        std::string title;
        std::string IP;
        std::string name;
        std::string pwd;
        int port;
    }                    _accountData[MAX_ACCOUNT_DATA];
    bool                 _pwdCheckBoxChecked;
    int                  _lastTargetIndex;


    Form_Qt(Front_Qt_API * front)
        : QWidget()
        , _front(front)
        , _width(400)
        , _height(300)
        , _formLayout(this)
        , _IPCombobox(this)
        , _accountNB(0)
        , _IPField("", this)
        , _userNameField("", this)
        , _PWDField("", this)
        , _portField("", this)
        , _IPLabel(      QString("IP serveur :"), this)
        , _userNameLabel(QString("User name : "), this)
        , _PWDLabel(     QString("Password :  "), this)
        , _portLabel(    QString("Port :      "), this)
        , _errorLabel(   QString(""            ), this)
        , _pwdCheckBox(QString("Save password."), this)
        , _buttonConnexion("Connection", this)
        , _buttonOptions("Options", this)
        , _buttonReplay("Replay", this)
        , _pwdCheckBoxChecked(false)
        , _lastTargetIndex(0)
    {
        this->setWindowTitle("Remote Desktop Player");
        this->setAttribute(Qt::WA_DeleteOnClose);
        this->setFixedSize(this->_width, this->_height);

        this->setAccountData();

        if (this->_pwdCheckBoxChecked) {
            this->_pwdCheckBox.setCheckState(Qt::Checked);
        }
        //this->_IPField.setCompleter(this->_completer);
        this->_IPCombobox.setLineEdit(&(this->_IPField));
        //this->_IPCombobox.setCompleter(this->_completer);
        this->QObject::connect(&(this->_IPCombobox), SIGNAL(currentIndexChanged(int)) , this, SLOT(targetPicked(int)));

        this->_PWDField.setEchoMode(QLineEdit::Password);
        this->_PWDField.setInputMethodHints(Qt::ImhHiddenText | Qt::ImhNoPredictiveText | Qt::ImhNoAutoUppercase);
        this->_formLayout.addRow(&(this->_IPLabel)      , &(this->_IPCombobox));
        this->_formLayout.addRow(&(this->_userNameLabel), &(this->_userNameField));
        this->_formLayout.addRow(&(this->_PWDLabel)     , &(this->_PWDField));
        this->_formLayout.addRow(&(this->_portLabel)    , &(this->_portField));
        this->_formLayout.addRow(&(this->_pwdCheckBox));
        this->_formLayout.addRow(&(this->_errorLabel));
        this->setLayout(&(this->_formLayout));

        QRect rectReplay(QPoint(10, 226), QSize(110, 24));
        this->_buttonReplay.setToolTip(this->_buttonReplay.text());
        this->_buttonReplay.setGeometry(rectReplay);
        this->_buttonReplay.setCursor(Qt::PointingHandCursor);
        this->QObject::connect(&(this->_buttonReplay)   , SIGNAL (pressed()),  this, SLOT (replayPressed()));
        this->_buttonReplay.setFocusPolicy(Qt::StrongFocus);
        this->_buttonReplay.setAutoDefault(true);

        QRect rectConnexion(QPoint(280, 256), QSize(110, 24));
        this->_buttonConnexion.setToolTip(this->_buttonConnexion.text());
        this->_buttonConnexion.setGeometry(rectConnexion);
        this->_buttonConnexion.setCursor(Qt::PointingHandCursor);
        //this->QObject::connect(&(this->_buttonConnexion)   , SIGNAL (pressed()),  this, SLOT (connexionPressed()));
        this->QObject::connect(&(this->_buttonConnexion)   , SIGNAL (released()), this, SLOT (connexionReleased()));
        this->_buttonConnexion.setFocusPolicy(Qt::StrongFocus);
        this->_buttonConnexion.setAutoDefault(true);

        QRect rectOptions(QPoint(10, 256), QSize(110, 24));
        this->_buttonOptions.setToolTip(this->_buttonOptions.text());
        this->_buttonOptions.setGeometry(rectOptions);
        this->_buttonOptions.setCursor(Qt::PointingHandCursor);
        this->QObject::connect(&(this->_buttonOptions)     , SIGNAL (pressed()),  this, SLOT (optionsPressed()));
        this->QObject::connect(&(this->_buttonOptions)     , SIGNAL (released()), this, SLOT (optionsReleased()));
        this->_buttonOptions.setFocusPolicy(Qt::StrongFocus);

        QDesktopWidget* desktop = QApplication::desktop();
        int centerW = (desktop->width()/2)  - (this->_width/2);
        int centerH = (desktop->height()/2) - (this->_height/2);
        this->move(centerW, centerH);
    }

    ~Form_Qt() {}

    void setAccountData() {
        this->_accountNB = 0;
        std::ifstream ifichier(this->_front->USER_CONF_LOG, std::ios::in);

        if (ifichier) {
            int accountNB(0);
            std::string ligne;
            const std::string delimiter = " ";

            while(std::getline(ifichier, ligne)) {

                auto pos(ligne.find(delimiter));
                std::string tag  = ligne.substr(0, pos);
                std::string info = ligne.substr(pos + delimiter.length(), ligne.length());

                if (tag.compare(std::string("save_pwd")) == 0) {
                    if (info.compare(std::string("true")) == 0) {
                        this->_pwdCheckBoxChecked = true;
                    } else {
                        this->_pwdCheckBoxChecked = false;
                    }
                } else
                if (tag.compare(std::string("last_target")) == 0) {
                    this->_lastTargetIndex = std::stoi(info);
                } else
                if (tag.compare(std::string("title")) == 0) {
                    this->_accountData[accountNB].title = info;
                } else
                if (tag.compare(std::string("IP")) == 0) {
                    this->_accountData[accountNB].IP = info;
                } else
                if (tag.compare(std::string("name")) == 0) {
                    this->_accountData[accountNB].name = info;
                } else
                if (tag.compare(std::string("pwd")) == 0) {
                    this->_accountData[accountNB].pwd = info;
                } else
                if (tag.compare(std::string("port")) == 0) {
                    this->_accountData[accountNB].port = std::stoi(info);
                    accountNB++;
                    if (accountNB == MAX_ACCOUNT_DATA) {
                        this->_accountNB = MAX_ACCOUNT_DATA;
                        accountNB = 0;
                    }
                }
            }

            if (this->_accountNB < MAX_ACCOUNT_DATA) {
                this->_accountNB = accountNB;
            }

            this->_IPCombobox.clear();
            this->_IPCombobox.addItem(QString(""), 0);
            QStringList stringList;
            for (int i = 0; i < this->_accountNB; i++) {
                std::string title(this->_accountData[i].IP + std::string(" - ")+ this->_accountData[i].name);
                this->_IPCombobox.addItem(QString(title.c_str()), i+1);
                stringList << title.c_str();
            }
            this->_completer = new QCompleter(stringList, this);
        }
     }

    void set_ErrorMsg(std::string str) {
        this->_errorLabel.clear();
        this->_errorLabel.setText(QString(str.c_str()));
    }

    void set_IPField(std::string str) {
        this->_IPField.clear();
        this->_IPField.insert(QString(str.c_str()));
    }

    void set_userNameField(std::string str) {
        this->_userNameField.clear();
        this->_userNameField.insert(QString(str.c_str()));
    }

    void set_PWDField(std::string str) {
        this->_PWDField.clear();
        this->_PWDField.insert(QString(str.c_str()));
    }

    void set_portField(int str) {
        this->_portField.clear();
        if (str == 0) {
            this->_portField.insert(QString(""));
        } else {
            this->_portField.insert(QString(std::to_string(str).c_str()));
        }
    }

    std::string get_IPField() {
        std::string delimiter(" - ");
        std::string ip_field_content = this->_IPField.text().toStdString();
        auto pos(ip_field_content.find(delimiter));
        std::string IP  = ip_field_content.substr(0, pos);
        return IP;
    }

    std::string get_userNameField() {
        return this->_userNameField.text().toStdString();
    }

    std::string get_PWDField() {
        return this->_PWDField.text().toStdString();
    }

    int get_portField() {
        return this->_portField.text().toInt();
    }



private Q_SLOTS:
    void targetPicked(int index) {
        if (index == 0) {
            this->_IPField.clear();
            this->_userNameField.clear();
            this->_PWDField.clear();
            this->_portField.clear();

        } else {
            index--;
            this->set_IPField(this->_accountData[index].IP);
            this->set_userNameField(this->_accountData[index].name);
            this->set_PWDField(this->_accountData[index].pwd);
            this->set_portField(this->_accountData[index].port);
        }

        this->_buttonConnexion.setFocus();
    }

    void replayPressed() {
        QString filePath("");
        filePath = QFileDialog::getOpenFileName(this, tr("Open a Movie"),
                                                this->_front->REPLAY_DIR.c_str(),
                                                tr("Movie Files(*.mwrm)"));
        std::string str_movie_path(filePath.toStdString());
        this->_front->replay(str_movie_path);
    }

    void connexionReleased() {

        if (this->_front->connexionReleased()) {
            bool alreadySet = false;
            this->_pwdCheckBoxChecked = this->_pwdCheckBox.isChecked();

            std::string title(this->get_IPField() + std::string(" - ")+ this->get_userNameField());

            for (int i = 0; i < this->_accountNB; i++) {

                if (this->_accountData[i].title.compare(title) == 0) {
                    alreadySet = true;
                    this->_lastTargetIndex = i;
                    this->_accountData[i].pwd  = this->get_PWDField();
                    this->_accountData[i].port = this->get_portField();
                }
            }
            if (!alreadySet) {
                this->_accountData[this->_accountNB].title = title;
                this->_accountData[this->_accountNB].IP    = this->get_IPField();
                this->_accountData[this->_accountNB].name  = this->get_userNameField();
                this->_accountData[this->_accountNB].pwd   = this->get_PWDField();
                this->_accountData[this->_accountNB].port  = this->get_portField();
                this->_accountNB++;
                this->_lastTargetIndex = this->_accountNB;
                if (this->_accountNB > MAX_ACCOUNT_DATA) {
                    this->_accountNB = MAX_ACCOUNT_DATA;
                }
            }

            std::ofstream ofichier(this->_front->USER_CONF_LOG, std::ios::out | std::ios::trunc);
            if(ofichier) {

                if (this->_pwdCheckBoxChecked) {
                    ofichier << "save_pwd true" << "\n";
                } else {
                    ofichier << "save_pwd false" << "\n";
                }
                ofichier << "last_target " <<  this->_lastTargetIndex << "\n";

                ofichier << "\n";

                for (int i = 0; i < this->_accountNB; i++) {
                    ofichier << "title " << this->_accountData[i].title << "\n";
                    ofichier << "IP "    << this->_accountData[i].IP    << "\n";
                    ofichier << "name "  << this->_accountData[i].name  << "\n";
                    if (this->_pwdCheckBoxChecked) {
                        ofichier << "pwd " << this->_accountData[i].pwd << "\n";
                    } else {
                        ofichier << "pwd " << "\n";
                    }
                    ofichier << "port " << this->_accountData[i].port << "\n";
                    ofichier << "\n";
                }
            }
        }
    }

    void optionsPressed() {}

    void optionsReleased() {
        this->_front->options();
        //new DialogOptions_Qt(this->_front, this);
    }
};


class Screen_Qt : public QWidget
{

Q_OBJECT

public:
    Front_Qt_API  * _front;
    QPushButton     _buttonCtrlAltDel;
    QPushButton     _buttonRefresh;
    QPushButton     _buttonDisconnexion;
    QColor          _penColor;
    QPixmap      * _cache;
    QPainter       _cache_painter;
    QPixmap      * _trans_cache;
    QPainter       _trans_cache_painter;
    int            _width;
    int            _height;
    QPixmap        _match_pixmap;
    bool           _connexionLasted;
    QTimer         _timer;
    QTimer         _timer_replay;
    uint8_t        _screen_index;

    bool           _running;
    std::string    _movie_name;

    enum : int {
        BUTTON_HEIGHT = 20
    };

    uchar cursor_data[Pointer::DATA_SIZE*4];
    int cursorHotx;
    int cursorHoty;
    bool mouse_out;

    Screen_Qt (Front_Qt_API * front, int screen_index, QPixmap * cache, QPixmap * trans_cache)
        : QWidget()
        , _front(front)
        , _buttonCtrlAltDel("CTRL + ALT + DELETE", this)
        , _buttonRefresh("Refresh", this)
        , _buttonDisconnexion("Disconnection", this)
        , _penColor(Qt::black)
        , _cache(cache)
        , _trans_cache(trans_cache)
        , _width(this->_front->info.width)
        , _height(this->_front->info.height)
        , _match_pixmap(this->_width, this->_height)
        , _connexionLasted(false)
        , _screen_index(screen_index)
        , _running(false)
        , cursorHotx(0)
        , cursorHoty(0)
        , mouse_out(false)
    {
        this->setMouseTracking(true);
        this->installEventFilter(this);
        this->setAttribute(Qt::WA_DeleteOnClose);
        std::string screen_index_str = std::to_string(int(this->_screen_index));
        std::string title = "Remote Desktop Player connected to [" + this->_front->target_IP +  "]. " + screen_index_str;
        this->setWindowTitle(QString(title.c_str()));

        if (this->_front->is_spanning) {
            this->setWindowState(Qt::WindowFullScreen);
            //this->_height -= 2*Front_Qt_API::BUTTON_HEIGHT;
        } else {
            this->setFixedSize(this->_width, this->_height + BUTTON_HEIGHT);
        }

        QDesktopWidget * desktop = QApplication::desktop();
        int shift(10 * this->_screen_index);
        uint32_t centerW = (desktop->width()/2)  - (this->_width/2);
        uint32_t centerH = (desktop->height()/2) - ((this->_height+BUTTON_HEIGHT)/2);
        if (this->_front->is_spanning) {
            centerW = 0;
            centerH = 0;
        }
        this->move(centerW + shift, centerH + shift);

        this->setFocusPolicy(Qt::StrongFocus);
    }

    Screen_Qt (Front_Qt_API * front, QPixmap * cache, std::string const & movie_path, QPixmap * trans_cache)
        : QWidget()
        , _front(front)
        , _buttonCtrlAltDel("Play", this)
        , _buttonRefresh("Stop", this)
        , _buttonDisconnexion("Close", this)
        , _penColor(Qt::black)
        , _cache(cache)
        , _cache_painter(this->_cache)
        , _trans_cache(trans_cache)
        , _trans_cache_painter(this->_trans_cache)
        , _width(this->_front->info.width)
        , _height(this->_front->info.height)
        , _match_pixmap(this->_width, this->_height)
        , _connexionLasted(false)
        , _timer_replay(this)
        , _screen_index(0)
        , _running(false)
        , _movie_name(movie_path)
        , cursorHotx(0)
        , cursorHoty(0)
        , mouse_out(false)
    {
        std::string title = "Remote Desktop Player " + this->_movie_name;
        this->setWindowTitle(QString(title.c_str()));
        this->setAttribute(Qt::WA_DeleteOnClose);
        this->paintCache().fillRect(0, 0, this->_width, this->_height, {0, 0, 0});

        if (this->_front->is_spanning) {
            this->setWindowState(Qt::WindowFullScreen);
        } else {
            this->setFixedSize(this->_width, this->_height + BUTTON_HEIGHT);
        }

        QRect rectCtrlAltDel(QPoint(0, this->_height+1),QSize(this->_width/3, BUTTON_HEIGHT));
        this->_buttonCtrlAltDel.setToolTip(this->_buttonCtrlAltDel.text());
        this->_buttonCtrlAltDel.setGeometry(rectCtrlAltDel);
        this->_buttonCtrlAltDel.setCursor(Qt::PointingHandCursor);
        this->QObject::connect(&(this->_buttonCtrlAltDel)     , SIGNAL (pressed()),  this, SLOT (playPressed()));
        this->_buttonCtrlAltDel.setFocusPolicy(Qt::NoFocus);

        QRect rectRefresh(QPoint(this->_width/3, this->_height+1),QSize(this->_width/3, BUTTON_HEIGHT));
        this->_buttonRefresh.setToolTip(this->_buttonRefresh.text());
        this->_buttonRefresh.setGeometry(rectRefresh);
        this->_buttonRefresh.setCursor(Qt::PointingHandCursor);
        this->QObject::connect(&(this->_buttonRefresh), SIGNAL (pressed()), this, SLOT (stopRelease()));
        this->_buttonRefresh.setFocusPolicy(Qt::NoFocus);

        QRect rectDisconnexion(QPoint(((this->_width/3)*2), this->_height+1),QSize(this->_width-((this->_width/3)*2), BUTTON_HEIGHT));
        this->_buttonDisconnexion.setToolTip(this->_buttonDisconnexion.text());
        this->_buttonDisconnexion.setGeometry(rectDisconnexion);
        this->_buttonDisconnexion.setCursor(Qt::PointingHandCursor);
        this->QObject::connect(&(this->_buttonDisconnexion), SIGNAL (released()), this, SLOT (closeReplay()));
        this->_buttonDisconnexion.setFocusPolicy(Qt::NoFocus);

        uint32_t centerW = 0;
        uint32_t centerH = 0;
        if (!this->_front->is_spanning) {
            QDesktopWidget* desktop = QApplication::desktop();
            centerW = (desktop->width()/2)  - (this->_width/2);
            centerH = (desktop->height()/2) - ((this->_height+BUTTON_HEIGHT)/2);
        }
        this->move(centerW, centerH);

        this->QObject::connect(&(this->_timer_replay), SIGNAL (timeout()),  this, SLOT (playReplay()));

        this->setFocusPolicy(Qt::StrongFocus);
    }

    Screen_Qt (Front_Qt_API * front, QPixmap * cache, QPixmap * trans_cache)
        : QWidget()
        , _front(front)
        , _buttonCtrlAltDel("CTRL + ALT + DELETE", this)
        , _buttonRefresh("Refresh", this)
        , _buttonDisconnexion("Disconnection", this)
        , _penColor(Qt::black)
        , _cache(cache)
        , _cache_painter(this->_cache)
        , _trans_cache(trans_cache)
        , _width(this->_front->info.width)
        , _height(this->_front->info.height)
        , _match_pixmap(this->_width, this->_height)
        , _connexionLasted(false)
        , _screen_index(0)
        , _running(false)
        , cursorHotx(0)
        , cursorHoty(0)
        , mouse_out(false)
    {
        this->setMouseTracking(true);
        this->installEventFilter(this);
        this->setAttribute(Qt::WA_DeleteOnClose);
        std::string title = "Remote Desktop Player connected to [" + this->_front->target_IP +  "].";
        this->setWindowTitle(QString(title.c_str()));

        if (this->_front->is_spanning) {
            this->setWindowState(Qt::WindowFullScreen);
        } else {
            this->setFixedSize(this->_width, this->_height + BUTTON_HEIGHT);
        }

        QRect rectCtrlAltDel(QPoint(0, this->_height+1),QSize(this->_width/3, BUTTON_HEIGHT));
        this->_buttonCtrlAltDel.setToolTip(this->_buttonCtrlAltDel.text());
        this->_buttonCtrlAltDel.setGeometry(rectCtrlAltDel);
        this->_buttonCtrlAltDel.setCursor(Qt::PointingHandCursor);
        this->QObject::connect(&(this->_buttonCtrlAltDel)  , SIGNAL (pressed()),  this, SLOT (CtrlAltDelPressed()));
        this->QObject::connect(&(this->_buttonCtrlAltDel)  , SIGNAL (released()), this, SLOT (CtrlAltDelReleased()));;
        this->_buttonCtrlAltDel.setFocusPolicy(Qt::NoFocus);

        QRect rectRefresh(QPoint(this->_width/3, this->_height+1),QSize(this->_width/3, BUTTON_HEIGHT));
        this->_buttonRefresh.setToolTip(this->_buttonRefresh.text());
        this->_buttonRefresh.setGeometry(rectRefresh);
        this->_buttonRefresh.setCursor(Qt::PointingHandCursor);
        this->QObject::connect(&(this->_buttonRefresh)     , SIGNAL (pressed()),  this, SLOT (RefreshPressed()));
        this->_buttonRefresh.setFocusPolicy(Qt::NoFocus);

        QRect rectDisconnexion(QPoint(((this->_width/3)*2), this->_height+1),QSize(this->_width-((this->_width/3)*2), BUTTON_HEIGHT));
        this->_buttonDisconnexion.setToolTip(this->_buttonDisconnexion.text());
        this->_buttonDisconnexion.setGeometry(rectDisconnexion);
        this->_buttonDisconnexion.setCursor(Qt::PointingHandCursor);
        this->QObject::connect(&(this->_buttonDisconnexion), SIGNAL (released()), this, SLOT (disconnexionRelease()));
        this->_buttonDisconnexion.setFocusPolicy(Qt::NoFocus);

        uint32_t centerW = 0;
        uint32_t centerH = 0;
        if (!this->_front->is_spanning) {
            QDesktopWidget* desktop = QApplication::desktop();
            centerW = (desktop->width()/2)  - (this->_width/2);
            centerH = (desktop->height()/2) - ((this->_height+BUTTON_HEIGHT)/2);
        }
        this->move(centerW, centerH);

        this->setFocusPolicy(Qt::StrongFocus);
    }

    ~Screen_Qt() {
        if (!this->_connexionLasted) {
            this->_front->closeFromScreen();
        }
    }

    void set_mem_cursor(const uchar * data, int x, int y) {
        this->cursorHotx = x;
        this->cursorHoty = y;
        for (int i = 0; i < Pointer::DATA_SIZE*4; i++) {
            this->cursor_data[i] = data[i];
        }
        this->update_current_cursor();
    }

    void update_current_cursor() {
        QImage image(this->cursor_data, 32, 32, QImage::Format_ARGB32_Premultiplied);
        QPixmap map = QPixmap::fromImage(image);
        QCursor qcursor(map, this->cursorHotx, this->cursorHoty);

        this->setCursor(qcursor);
    }

    void update_view() {
        this->slotRepaint();
    }

    void disconnection() {
        this->_connexionLasted = true;
    }

    QPainter & paintCache() {
        return this->_cache_painter;
    }

    QPainter & paintTransCache() {
        return this->_trans_cache_painter;
    }

    void paintEvent(QPaintEvent * event) {
        Q_UNUSED(event);

        QPen                 pen;
        QPainter             painter;
        painter.begin(this);
        painter.setRenderHint(QPainter::Antialiasing);
        pen.setWidth(1);
        pen.setBrush(this->_penColor);
        painter.setPen(pen);
        painter.drawPixmap(QPoint(0, 0), this->_match_pixmap, QRect(0, 0, this->_width, this->_height));
        painter.end();
    }

    QPixmap * getCache() {
        return this->_cache;
    }

    void setPenColor(QColor color) {
        this->_penColor = color;
    }


private:
    void mousePressEvent(QMouseEvent *e) {
        this->_front->mousePressEvent(e, 0);
    }

    void mouseReleaseEvent(QMouseEvent *e) {
        this->_front->mouseReleaseEvent(e, 0);
    }

    void keyPressEvent(QKeyEvent *e) {
        this->_front->keyPressEvent(e);
    }

    void keyReleaseEvent(QKeyEvent *e) {
        this->_front->keyReleaseEvent(e);
    }

    void wheelEvent(QWheelEvent *e) {
        this->_front->wheelEvent(e);
    }

    void enterEvent(QEvent *event) {
        Q_UNUSED(event);
        //this->update_current_cursor();
        //this->_front->_current_screen_index =  this->_screen_index;
    }

    bool eventFilter(QObject *obj, QEvent *e) {
        this->_front->eventFilter(obj, e, 0);
        return false;
    }


public Q_SLOTS:
    void playPressed() {
        if (this->_running) {
            this->_running = false;
            this->_buttonCtrlAltDel.setText("Play");
            this->_timer_replay.stop();
        } else {
            this->_running = true;
            this->_buttonCtrlAltDel.setText("Pause");
            this->_timer_replay.start(1);
        }
    }

    void playReplay() {
        if (this->_front->replay_mod->play_qt()) {
            this->slotRepaint();
        }

        if (this->_front->replay_mod->get_break_privplay_qt()) {
            this->_timer_replay.stop();
            this->slotRepaint();
            this->_buttonCtrlAltDel.setText("Replay");
            this->_running = false;
            this->_front->load_replay_mod(this->_movie_name);
        }
    }

    void closeReplay() {
        this->_front->delete_replay_mod();
        this->_front->disconnexionReleased();
    }

    void slotRepaint() {

        QPainter match_painter(&(this->_match_pixmap));
        match_painter.drawPixmap(QPoint(0, 0), *(this->_cache), QRect(0, 0, this->_width, this->_height));
        match_painter.drawPixmap(QPoint(0, 0), *(this->_trans_cache), QRect(0, 0, this->_width, this->_height));
        this->repaint();
    }

    void RefreshPressed() {
        this->_front->RefreshPressed();
    }

    void CtrlAltDelPressed() {
        this->_front->CtrlAltDelPressed();
    }

    void CtrlAltDelReleased() {
        this->_front->CtrlAltDelReleased();
    }

    void stopRelease() {
        this->_buttonCtrlAltDel.setText("Replay");
        this->_timer_replay.stop();
        this->_running = false;
        this->_front->load_replay_mod(this->_movie_name);
        this->_cache_painter.fillRect(0, 0, this->_width, this->_height, Qt::black);
        this->slotRepaint();
    }

    void disconnexionRelease(){
        this->_front->disconnexionReleased();
    }

    void setMainScreenOnTopRelease() {
        this->_front->setMainScreenOnTopRelease();
    }
};




class FrontQtRDPGraphicAPI : public Front_Qt_API
{
    struct Snapshoter : gdi::CaptureApi
    {
        FrontQtRDPGraphicAPI & front;

        Snapshoter(FrontQtRDPGraphicAPI & front) : front(front) {}

        std::chrono::microseconds do_snapshot(
            const timeval& /*now*/, int cursor_x, int cursor_y, bool /*ignore_frame_in_timeval*/
        ) override {
            this->front.update_pointer_position(cursor_x, cursor_y);
            std::chrono::microseconds res(1);
            return res;
        }
    };
    Snapshoter snapshoter;

public:

    // Graphic members
    int                  mod_bpp;
    BGRPalette           mod_palette;
    Form_Qt            * form;
    Screen_Qt          * screen;
    Mod_Qt             * mod_qt;
    QPixmap            * cache;
    QPixmap            * trans_cache;
    gdi::GraphicApi    * graph_capture;

    struct MouseData {
        QImage cursor_image;
        uint16_t x = 0;
        uint16_t y = 0;
    } mouse_data;

    // Connexion socket members
    int                  _timer;
    bool                 connected;
     std::unique_ptr<Capture>  capture;
    Font                 _font;
    std::string          _error;

    // Keyboard Controllers members
    Keymap2              keymap;
    bool                 ctrl_alt_delete; // currently not used and always false
    StaticOutStream<256> decoded_data;    // currently not initialised
    uint8_t              keyboardMods;

    CHANNELS::ChannelDefArray   cl;



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //------------------------
    //      CONSTRUCTOR
    //------------------------

    FrontQtRDPGraphicAPI( RDPVerbose verbose)
      : Front_Qt_API(verbose)
      , snapshoter(*this)
      , mod_bpp(24)
      , mod_palette(BGRPalette::classic_332())
      , form(nullptr)
      , screen(nullptr)
      , mod_qt(nullptr)
      , cache(nullptr)
      , trans_cache(nullptr)
      , graph_capture(nullptr)
      , _timer(0)
      , _error("error")
      , keymap()
      , ctrl_alt_delete(false)
    {
        SSL_load_error_strings();
        SSL_library_init();

        // Windows and socket contrainer
        this->mod_qt = new Mod_Qt(this, this->form);
        this->form = new Form_Qt(this);

        if (this->mod_bpp == this->info.bpp) {
            this->mod_palette = BGRPalette::classic_332();
        }

        this->info.width  = 800;
        this->info.height = 600;
        this->info.keylayout = 0x040C;// 0x40C FR, 0x409 USA
        this->info.console_session = 0;
        this->info.brush_cache_code = 0;
        this->info.bpp = 24;
        this->imageFormatRGB  = this->bpp_to_QFormat(this->info.bpp, false);
        if (this->info.bpp ==  32) {
            this->imageFormatARGB = this->bpp_to_QFormat(this->info.bpp, true);
        }
        this->info.rdp5_performanceflags = PERF_DISABLE_WALLPAPER;
        this->info.cs_monitor.monitorCount = 1;

        this->qtRDPKeymap.setKeyboardLayout(this->info.keylayout);
        this->keymap.init_layout(this->info.keylayout);

        this->disconnect("");
    }


    virtual void begin_update() override {}

    virtual void end_update() override {
        this->screen->update_view();

        if (bool(this->verbose & RDPVerbose::graphics)) {
           LOG(LOG_INFO, "--------- FRONT ------------------------");
           LOG(LOG_INFO, "end_update");
           LOG(LOG_INFO, "========================================\n");
        }
        if (this->is_recording && !this->is_replaying) {
            this->graph_capture->end_update();
            struct timeval time;
            gettimeofday(&time, nullptr);
            this->capture.get()->periodic_snapshot(time, this->mouse_data.x, this->mouse_data.y, false);
        }
    }

    virtual void update_pointer_position(uint16_t xPos, uint16_t yPos) override {

        if (this->is_replaying) {
            this->trans_cache->fill(Qt::transparent);
            QRect nrect(xPos, yPos, this->mouse_data.cursor_image.width(), this->mouse_data.cursor_image.height());

            this->screen->paintTransCache().drawImage(nrect, this->mouse_data.cursor_image);
        }
    }

    virtual ResizeResult server_resize(int width, int height, int bpp) override{
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            LOG(LOG_INFO, "server_resize(width=%d, height=%d, bpp=%d)", width, height, bpp);
            LOG(LOG_INFO, "========================================\n");
        }

        bool remake_screen = (this->info.width != width || this->info.height != height);

        this->mod_bpp = bpp;
        this->info.bpp = bpp;
        this->info.width = width;
        this->info.height = height;
        this->imageFormatRGB  = this->bpp_to_QFormat(this->info.bpp, false);
        if (this->info.bpp ==  32) {
            this->imageFormatARGB = this->bpp_to_QFormat(this->info.bpp, true);
        }

        if (remake_screen) {
            if (this->screen) {
                this->screen->disconnection();
                this->dropScreen();
                this->cache = new QPixmap(this->info.width, this->info.height);
                this->trans_cache = new QPixmap(this->info.width, this->info.height);
                this->trans_cache->fill(Qt::transparent);
                this->screen = new Screen_Qt(this, this->cache, this->trans_cache);
                this->screen->show();
            }
        }

        return ResizeResult::no_need;
    }

    virtual void set_pointer(Pointer const & cursor) override {

        QImage image_data(cursor.data, cursor.width, cursor.height, this->bpp_to_QFormat(24, false));
        QImage image_mask(cursor.mask, cursor.width, cursor.height, QImage::Format_Mono);

        if (cursor.mask[0x48] == 0xFF &&
            cursor.mask[0x49] == 0xFF &&
            cursor.mask[0x4A] == 0xFF &&
            cursor.mask[0x4B] == 0xFF) {

            image_mask = image_data.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            image_data.invertPixels();

        } else {
            image_mask.invertPixels();
        }

        image_data = image_data.mirrored(false, true).convertToFormat(QImage::Format_ARGB32_Premultiplied);
        image_mask = image_mask.mirrored(false, true).convertToFormat(QImage::Format_ARGB32_Premultiplied);

        const uchar * data_data = image_data.bits();
        const uchar * mask_data = image_mask.bits();

        uint8_t data[Pointer::DATA_SIZE*4];

        for (int i = 0; i < Pointer::DATA_SIZE; i += 4) {
            data[i  ] = data_data[i+2];
            data[i+1] = data_data[i+1];
            data[i+2] = data_data[i  ];
            data[i+3] = mask_data[i+0];
        }

        if (this->is_replaying) {
            this->mouse_data.cursor_image = QImage(static_cast<uchar *>(data), cursor.x, cursor.y, QImage::Format_ARGB32_Premultiplied);

        } else {
            this->screen->set_mem_cursor(static_cast<uchar *>(data), cursor.x, cursor.y);

            if (this->is_recording) {
                this->graph_capture->set_pointer(cursor);
                struct timeval time;
                gettimeofday(&time, nullptr);
                this->capture.get()->periodic_snapshot(time, this->mouse_data.x, this->mouse_data.y, false);
            }
        }
    }

    void load_replay_mod(std::string const & movie_name) override {
        try {
            this->replay_mod.reset(new ReplayMod( *this
                                                , (this->REPLAY_DIR + "/").c_str()
                                                , movie_name.c_str()
                                                , 0             //this->info.width
                                                , 0             //this->info.height
                                                , this->_error
                                                , this->_font
                                                , true
                                                , to_verbose_flags(0)
                                                ));

            this->replay_mod->add_consumer(nullptr, &this->snapshoter, nullptr, nullptr, nullptr);
        } catch (const Error &) {
            LOG(LOG_WARNING, "new ReplayMod error %s", this->_error.c_str());
        }


    }

    void delete_replay_mod() override {
        this->replay_mod.reset();
    }

    uint32_t string_to_hex32(unsigned char * str) {
        size_t size = sizeof(str);
        uint32_t hex32(0);
        for (size_t i = 0; i < size; i++) {
            int s = str[i];
            if(s > 47 && s < 58) {                      //this covers 0-9
                hex32 += (s - 48) << (size - i - 1);
            } else if (s > 64 && s < 71) {              // this covers A-F
                hex32 += (s - 55) << (size - i - 1);
            } else if (s > 'a'-1 && s < 'f'+1) {        // this covers a-f
                hex32 += (s - 'a') << (size - i - 1);
            }
        }

        return hex32;
    }



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //-----------------------
    //   GRAPHIC FUNCTIONS
    //-----------------------

    struct Op_0x11 {
        uchar op(const uchar src, const uchar dst) const {  // +------+-------------------------------+
             return ~(src | dst);                           // | 0x11 | ROP: 0x001100A6 (NOTSRCERASE) |
         }                                                  // |      | RPN: DSon                     |
     };                                                     // +------+-------------------------------+

    struct Op_0x22 {
        uchar op(const uchar src, const uchar dst) const {  // +------+-------------------------------+
             return (~src & dst);                           // | 0x22 | ROP: 0x00220326               |
         }                                                  // |      | RPN: DSna                     |
     };                                                     // +------+-------------------------------+

     struct Op_0x33 {
        uchar op(const uchar src, const uchar) const {      // +------+-------------------------------+
             return (~src);                                 // | 0x33 | ROP: 0x00330008 (NOTSRCCOPY)  |
        }                                                   // |      | RPN: Sn                       |
     };                                                     // +------+-------------------------------+

     struct Op_0x44 {
        uchar op(const uchar src, const uchar dst) const {  // +------+-------------------------------+
            return (src & ~dst);                            // | 0x44 | ROP: 0x00440328 (SRCERASE)    |
        }                                                   // |      | RPN: SDna                     |
    };                                                      // +------+-------------------------------+

    struct Op_0x55 {
        uchar op(const uchar, const uchar dst) const {      // +------+-------------------------------+
             return (~dst);                                 // | 0x55 | ROP: 0x00550009 (DSTINVERT)   |
        }                                                   // |      | RPN: Dn                       |
     };                                                     // +------+-------------------------------+

    struct Op_0x66 {
         uchar op(const uchar src, const uchar dst) const { // +------+-------------------------------+
            return (src ^ dst);                             // | 0x66 | ROP: 0x00660046 (SRCINVERT)   |
         }                                                  // |      | RPN: DSx                      |
     };                                                     // +------+-------------------------------+

     struct Op_0x77 {
         uchar op(const uchar src, const uchar dst) const { // +------+-------------------------------+
             return ~(src & dst);                           // | 0x77 | ROP: 0x007700E6               |
         }                                                  // |      | RPN: DSan                     |
     };                                                     // +------+-------------------------------+

    struct Op_0x88 {
         uchar op(const uchar src, const uchar dst) const { // +------+-------------------------------+
            return (src & dst);                             // | 0x88 | ROP: 0x008800C6 (SRCAND)      |
         }                                                  // |      | RPN: DSa                      |
     };                                                     // +------+-------------------------------+

     struct Op_0x99 {
        uchar op(const uchar src, const uchar dst) const {  // +------+-------------------------------+
            return ~(src ^ dst);                            // | 0x99 | ROP: 0x00990066               |
        }                                                   // |      | RPN: DSxn                     |
     };                                                     // +------+-------------------------------+

     struct Op_0xBB {
        uchar op(const uchar src, const uchar dst) const {  // +------+-------------------------------+
            return (~src | dst);                            // | 0xBB | ROP: 0x00BB0226 (MERGEPAINT)  |
        }                                                   // |      | RPN: DSno                     |
     };                                                     // +------+-------------------------------+

     struct Op_0xDD {
        uchar op(const uchar src, const uchar dst) const {  // +------+-------------------------------+
            return (src | ~dst);                            // | 0xDD | ROP: 0x00DD0228               |
        }                                                   // |      | RPN: SDno                     |
     };                                                     // +------+-------------------------------+

    struct Op_0xEE {
        uchar op(const uchar src, const uchar dst) const {  // +------+-------------------------------+
            return (src | dst);                             // | 0xEE | ROP: 0x00EE0086 (SRCPAINT)    |
        }                                                   // |      | RPN: DSo                      |
    };                                                      // +------+-------------------------------+


    template<class Op>
    void draw_memblt_op(const Rect & drect, const Bitmap & bitmap) {
        const uint16_t mincx = std::min<int16_t>(bitmap.cx(), std::min<int16_t>(this->info.width - drect.x, drect.cx));
        const uint16_t mincy = std::min<int16_t>(bitmap.cy(), std::min<int16_t>(this->info.height - drect.y, drect.cy));

        if (mincx <= 0 || mincy <= 0) {
            return;
        }

        int rowYCoord(drect.y + drect.cy-1);

        QImage::Format format(this->bpp_to_QFormat(bitmap.bpp(), false)); //bpp
        QImage srcBitmap(bitmap.data(), mincx, mincy, bitmap.line_size(), format);
        QImage dstBitmap(this->screen->getCache()->toImage().copy(drect.x, drect.y, mincx, mincy));

        if (bitmap.bpp() == 24) {
            srcBitmap = srcBitmap.rgbSwapped();
        }

        if (bitmap.bpp() != this->info.bpp) {
            srcBitmap = srcBitmap.convertToFormat(this->imageFormatRGB);
        }
        dstBitmap = dstBitmap.convertToFormat(srcBitmap.format());

        int indice(mincy-1);

        std::unique_ptr<uchar[]> data = std::make_unique<uchar[]>(srcBitmap.bytesPerLine());

        for (size_t k = 0 ; k < mincy; k++) {

            const uchar * srcData = srcBitmap.constScanLine(k);
            const uchar * dstData = dstBitmap.constScanLine(indice - k);

            Op op;
            for (int i = 0; i < srcBitmap.bytesPerLine(); i++) {
                data[i] = op.op(srcData[i], dstData[i]);
            }

            QImage image(data.get(), mincx, 1, srcBitmap.format());
            QRect trect(drect.x, rowYCoord, mincx, 1);
            this->screen->paintCache().drawImage(trect, image);

            rowYCoord--;
        }
    }

    void draw_MemBlt(const Rect & drect, const Bitmap & bitmap, bool invert, int srcx, int srcy) {
        const int16_t mincx = bitmap.cx();
        const int16_t mincy = bitmap.cy();

        if (mincx <= 0 || mincy <= 0) {
            return;
        }

        const unsigned char * row = bitmap.data();

        QImage qbitmap(row, mincx, mincy, this->bpp_to_QFormat(bitmap.bpp(), false));

        qbitmap = qbitmap.mirrored(false, true);

        qbitmap = qbitmap.copy(srcx, srcy, drect.cx, drect.cy);

        if (invert) {
            qbitmap.invertPixels();
        }

        if (bitmap.bpp() == 24) {
            qbitmap = qbitmap.rgbSwapped();
        }

        const QRect trect(drect.x, drect.y, drect.cx, drect.cy);
        this->screen->paintCache().drawImage(trect, qbitmap);
    }


    void draw_RDPScrBlt(int srcx, int srcy, const Rect & drect, bool invert) {
        QImage qbitmap(this->screen->getCache()->toImage().copy(srcx, srcy, drect.cx, drect.cy));
        if (invert) {
            qbitmap.invertPixels();
        }
        const QRect trect(drect.x, drect.y, drect.cx, drect.cy);
        this->screen->paintCache().drawImage(trect, qbitmap);
    }


    QColor u32_to_qcolor(uint32_t color) {
        uint8_t b(color >> 16);
        uint8_t g(color >> 8);
        uint8_t r(color);
        //std::cout <<  "r=" <<  int(r) <<  " g=" <<  int(g) << " b=" <<  int(b) <<  std::endl;
        return {r, g, b};
    }

    QColor u32_to_qcolor_r(uint32_t color) {
        uint8_t b(color >> 16);
        uint8_t g(color >> 8);
        uint8_t r(color);
        //std::cout <<  "r=" <<  int(r) <<  " g=" <<  int(g) << " b=" <<  int(b) <<  std::endl;
        return {b, g, r};
    }


    QImage::Format bpp_to_QFormat(int bpp, bool alpha) {
        QImage::Format format(QImage::Format_RGB16);

        if (alpha) {

            switch (bpp) {
                case 15: format = QImage::Format_ARGB4444_Premultiplied; break;
                case 16: format = QImage::Format_ARGB4444_Premultiplied; break;
                case 24: format = QImage::Format_ARGB8565_Premultiplied; break;
                case 32: format = QImage::Format_ARGB32_Premultiplied;   break;
                default : break;
            }
        } else {

            switch (bpp) {
                case 15: format = QImage::Format_RGB555; break;
                case 16: format = QImage::Format_RGB16;  break;
                case 24: format = QImage::Format_RGB888; break;
                case 32: format = QImage::Format_RGB32;  break;
                default : break;
            }
        }

        return format;
    }

    void draw_RDPPatBlt(const Rect & rect, const QColor color, const QPainter::CompositionMode mode, const Qt::BrushStyle style) {
        QBrush brush(color, style);
        this->screen->paintCache().setBrush(brush);
        this->screen->paintCache().setCompositionMode(mode);
        this->screen->paintCache().drawRect(rect.x, rect.y, rect.cx, rect.cy);
        this->screen->paintCache().setCompositionMode(QPainter::CompositionMode_SourceOver);
        this->screen->paintCache().setBrush(Qt::SolidPattern);
    }

    void draw_RDPPatBlt(const Rect & rect, const QPainter::CompositionMode mode) {
        this->screen->paintCache().setCompositionMode(mode);
        this->screen->paintCache().drawRect(rect.x, rect.y, rect.cx, rect.cy);
        this->screen->paintCache().setCompositionMode(QPainter::CompositionMode_SourceOver);
    }



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //-----------------------------
    //       DRAW FUNCTIONS
    //-----------------------------

    using Front_Qt_API::draw;

    void draw(const RDPPatBlt & cmd, Rect clip, gdi::ColorCtx color_ctx) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }
        (void) color_ctx;
        //std::cout << "RDPPatBlt " << std::hex << static_cast<int>(cmd.rop) << std::endl;
        RDPPatBlt new_cmd24 = cmd;
        new_cmd24.back_color = color_decode_opaquerect(cmd.back_color, this->mod_bpp, this->mod_palette);
        new_cmd24.fore_color = color_decode_opaquerect(cmd.fore_color, this->mod_bpp, this->mod_palette);
        const Rect rect = clip.intersect(this->info.width, this->info.height).intersect(cmd.rect);

        QColor backColor = this->u32_to_qcolor(new_cmd24.back_color);
        QColor foreColor = this->u32_to_qcolor(new_cmd24.fore_color);

        if (cmd.brush.style == 0x03 && (cmd.rop == 0xF0 || cmd.rop == 0x5A)) { // external

            switch (cmd.rop) {

                // +------+-------------------------------+
                // | 0x5A | ROP: 0x005A0049 (PATINVERT)   |
                // |      | RPN: DPx                      |
                // +------+-------------------------------+
                case 0x5A:
                    {
                        QBrush brush(backColor, Qt::Dense4Pattern);
                        this->screen->paintCache().setBrush(brush);
                        this->screen->paintCache().setCompositionMode(QPainter::RasterOp_SourceXorDestination);
                        this->screen->paintCache().drawRect(rect.x, rect.y, rect.cx, rect.cy);
                        this->screen->paintCache().setCompositionMode(QPainter::CompositionMode_SourceOver);
                        this->screen->paintCache().setBrush(Qt::SolidPattern);
                    }
                    break;

                // +------+-------------------------------+
                // | 0xF0 | ROP: 0x00F00021 (PATCOPY)     |
                // |      | RPN: P                        |
                // +------+-------------------------------+
                case 0xF0:
                    {
                        QBrush brush(foreColor, Qt::Dense4Pattern);
                        this->screen->paintCache().setPen(Qt::NoPen);
                        this->screen->paintCache().fillRect(rect.x, rect.y, rect.cx, rect.cy, backColor);
                        this->screen->paintCache().setBrush(brush);
                        this->screen->paintCache().drawRect(rect.x, rect.y, rect.cx, rect.cy);
                        this->screen->paintCache().setBrush(Qt::SolidPattern);
                    }
                    break;
                default: LOG(LOG_WARNING, "RDPPatBlt brush_style = 0x03 rop = %x", cmd.rop);
                    break;
            }

        } else {
            switch (cmd.rop) {

                case 0x00: // blackness
                    this->screen->paintCache().fillRect(rect.x, rect.y, rect.cx, rect.cy, Qt::black);
                    break;
                    // +------+-------------------------------+
                    // | 0x05 | ROP: 0x000500A9               |
                    // |      | RPN: DPon                     |
                    // +------+-------------------------------+

                    // +------+-------------------------------+
                    // | 0x0F | ROP: 0x000F0001               |
                    // |      | RPN: Pn                       |
                    // +------+-------------------------------+
                case 0x0F:
                    this->draw_RDPPatBlt(rect, QPainter::RasterOp_NotSource);
                    break;
                    // +------+-------------------------------+
                    // | 0x50 | ROP: 0x00500325               |
                    // |      | RPN: PDna                     |
                    // +------+-------------------------------+
                case 0x50:
                    this->draw_RDPPatBlt(rect, QPainter::RasterOp_NotSourceAndNotDestination);
                    break;
                    // +------+-------------------------------+
                    // | 0x55 | ROP: 0x00550009 (DSTINVERT)   |
                    // |      | RPN: Dn                       |
                    // +------+-------------------------------+
                /*case 0x55:
                    this->draw_RDPPatBlt(rect, QPainter::RasterOp_NotDestination);

                    break;*/
                    // +------+-------------------------------+
                    // | 0x5A | ROP: 0x005A0049 (PATINVERT)   |
                    // |      | RPN: DPx                      |
                    // +------+-------------------------------+
                case 0x5A:
                    this->draw_RDPPatBlt(rect, QPainter::RasterOp_SourceXorDestination);
                    break;
                    // +------+-------------------------------+
                    // | 0x5F | ROP: 0x005F00E9               |
                    // |      | RPN: DPan                     |
                    // +------+-------------------------------+

                    // +------+-------------------------------+
                    // | 0xA0 | ROP: 0x00A000C9               |
                    // |      | RPN: DPa                      |
                    // +------+-------------------------------+
                case 0xA0:
                    this->draw_RDPPatBlt(rect, QPainter::RasterOp_SourceAndDestination);
                    break;
                    // +------+-------------------------------+
                    // | 0xA5 | ROP: 0x00A50065               |
                    // |      | RPN: PDxn                     |
                    // +------+-------------------------------+
                /*case 0xA5:
                    // this->draw_RDPPatBlt(rect, QPainter::RasterOp_NotSourceXorNotDestination);
                    break;*/
                    // +------+-------------------------------+
                    // | 0xAA | ROP: 0x00AA0029               |
                    // |      | RPN: D                        |
                    // +------+-------------------------------+
                case 0xAA: // change nothing
                    break;
                    // +------+-------------------------------+
                    // | 0xAF | ROP: 0x00AF0229               |
                    // |      | RPN: DPno                     |
                    // +------+-------------------------------+
                /*case 0xAF:
                    //this->draw_RDPPatBlt(rect, QPainter::RasterOp_NotSourceOrDestination);
                    break;*/
                    // +------+-------------------------------+
                    // | 0xF0 | ROP: 0x00F00021 (PATCOPY)     |
                    // |      | RPN: P                        |
                    // +------+-------------------------------+
                case 0xF0:
                    this->screen->paintCache().setPen(Qt::NoPen);
                    this->screen->paintCache().fillRect(rect.x, rect.y, rect.cx, rect.cy, backColor);
                    this->screen->paintCache().drawRect(rect.x, rect.y, rect.cx, rect.cy);
                    break;
                    // +------+-------------------------------+
                    // | 0xF5 | ROP: 0x00F50225               |
                    // |      | RPN: PDno                     |
                    // +------+-------------------------------+
                //case 0xF5:
                    //this->draw_RDPPatBlt(rect, QPainter::RasterOp_SourceOrNotDestination);
                    //break;
                    // +------+-------------------------------+
                    // | 0xFA | ROP: 0x00FA0089               |
                    // |      | RPN: DPo                      |
                    // +------+-------------------------------+
                case 0xFA:
                    this->draw_RDPPatBlt(rect, QPainter::RasterOp_SourceOrDestination);
                    break;

                case 0xFF: // whiteness
                    this->screen->paintCache().fillRect(rect.x, rect.y, rect.cx, rect.cy, Qt::white);
                    break;
                default: LOG(LOG_WARNING, "RDPPatBlt rop = %x", cmd.rop);
                    break;
            }
        }

        if (this->is_recording && !this->is_replaying) {
            this->graph_capture->draw(cmd, clip, gdi::ColorCtx(gdi::Depth::from_bpp(this->info.bpp), &this->mod_palette));
            struct timeval time;
            gettimeofday(&time, nullptr);
            this->capture.get()->periodic_snapshot(time, this->mouse_data.x, this->mouse_data.y, false);
        }
    }


    void draw(const RDPOpaqueRect & cmd, Rect clip, gdi::ColorCtx color_ctx) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }
        (void) color_ctx;
        //std::cout << "RDPOpaqueRect" << std::endl;
        RDPOpaqueRect new_cmd24 = cmd;
        new_cmd24.color = color_decode_opaquerect(cmd.color, this->info.bpp, this->mod_palette);
        QColor qcolor(this->u32_to_qcolor(new_cmd24.color));
        Rect rect(new_cmd24.rect.intersect(clip));

        this->screen->paintCache().fillRect(rect.x, rect.y, rect.cx, rect.cy, qcolor);

        if (this->is_recording && !this->is_replaying) {
            this->graph_capture->draw(cmd, clip, gdi::ColorCtx(gdi::Depth::from_bpp(this->info.bpp), &this->mod_palette));
            struct timeval time;
            gettimeofday(&time, nullptr);
            this->capture.get()->periodic_snapshot(time, this->mouse_data.x, this->mouse_data.y, false);
        }
    }


    void draw(const RDPBitmapData & bitmap_data, const Bitmap & bmp) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            //bitmap_data.log(LOG_INFO, "FakeFront");
            LOG(LOG_INFO, "========================================\n");
        }
        //std::cout << "RDPBitmapData" << std::endl;
        if (!bmp.is_valid()){
            return;
        }

        Rect rectBmp( bitmap_data.dest_left, bitmap_data.dest_top,
                                (bitmap_data.dest_right - bitmap_data.dest_left + 1),
                                (bitmap_data.dest_bottom - bitmap_data.dest_top + 1));
        const Rect clipRect(0, 0, this->info.width, this->info.height);
        const Rect drect = rectBmp.intersect(clipRect);

        const int16_t mincx = std::min<int16_t>(bmp.cx(), std::min<int16_t>(this->info.width - drect.x, drect.cx));
        const int16_t mincy = std::min<int16_t>(bmp.cy(), std::min<int16_t>(this->info.height - drect.y, drect.cy));;

        if (mincx <= 0 || mincy <= 0) {
            return;
        }

        int rowYCoord(drect.y + drect.cy - 1);

        QImage::Format format(this->bpp_to_QFormat(bmp.bpp(), false)); //bpp
        QImage qbitmap(bmp.data(), mincx, mincy, bmp.line_size(), format);

        if (bmp.bpp() == 24) {
            qbitmap = qbitmap.rgbSwapped();
        }

        if (bmp.bpp() != this->info.bpp) {
            qbitmap = qbitmap.convertToFormat(this->imageFormatRGB);
        }

        for (size_t k = 0 ; k < drect.cy; k++) {

            QImage image(qbitmap.constScanLine(k), mincx, 1, qbitmap.format());
            QRect trect(drect.x, rowYCoord, mincx, 1);
            this->screen->paintCache().drawImage(trect, image);
            rowYCoord--;
        }

        if (this->is_recording && !this->is_replaying) {
            this->graph_capture->draw(bitmap_data, bmp);
            struct timeval time;
            gettimeofday(&time, nullptr);
            this->capture.get()->periodic_snapshot(time, this->mouse_data.x, this->mouse_data.y, false);
        }
    }


    void draw(const RDPLineTo & cmd, Rect clip, gdi::ColorCtx color_ctx) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }
        (void) color_ctx;
        //std::cout << "RDPLineTo" << std::endl;
        RDPLineTo new_cmd24 = cmd;
        new_cmd24.back_color = color_decode_opaquerect(cmd.back_color, 24, this->mod_palette);
        new_cmd24.pen.color  = color_decode_opaquerect(cmd.pen.color,  24, this->mod_palette);

        // TODO clipping
        this->screen->setPenColor(this->u32_to_qcolor(new_cmd24.back_color));

        this->screen->paintCache().drawLine(new_cmd24.startx, new_cmd24.starty, new_cmd24.endx, new_cmd24.endy);

        if (this->is_recording && !this->is_replaying) {
            this->graph_capture->draw(cmd, clip, gdi::ColorCtx(gdi::Depth::from_bpp(this->info.bpp), &this->mod_palette));
            struct timeval time;
            gettimeofday(&time, nullptr);
            this->capture.get()->periodic_snapshot(time, this->mouse_data.x, this->mouse_data.y, false);
        }
    }


    void draw(const RDPScrBlt & cmd, Rect clip) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        //std::cout << "RDPScrBlt" << std::endl;

        const Rect drect = clip.intersect(this->info.width, this->info.height).intersect(cmd.rect);
        if (drect.isempty()) {
            return;
        }

        int srcx(drect.x + cmd.srcx - cmd.rect.x);
        int srcy(drect.y + cmd.srcy - cmd.rect.y);

        switch (cmd.rop) {

            case 0x00: this->screen->paintCache().fillRect(drect.x, drect.y, drect.cx, drect.cy, Qt::black);
                break;

            case 0x55: this->draw_RDPScrBlt(srcx, srcy, drect, true);
                break;

            case 0xAA: // nothing to change
                break;

            case 0xCC: this->draw_RDPScrBlt(srcx, srcy, drect, false);
                break;

            case 0xFF:
                this->screen->paintCache().fillRect(drect.x, drect.y, drect.cx, drect.cy, Qt::white);
                break;
            default: LOG(LOG_WARNING, "DEFAULT: RDPScrBlt rop = %x", cmd.rop);
                break;
        }

        if (this->is_recording && !this->is_replaying) {
            this->graph_capture->draw(cmd, clip);
            struct timeval time;
            gettimeofday(&time, nullptr);
            this->capture.get()->periodic_snapshot(time, this->mouse_data.x, this->mouse_data.y, false);
        }
    }


    void draw(const RDPMemBlt & cmd, Rect clip, const Bitmap & bitmap) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }
        //std::cout << "RDPMemBlt (" << std::hex << static_cast<int>(cmd.rop) << ")" <<  std::dec <<  std::endl;
        const Rect drect = clip.intersect(cmd.rect);
        if (drect.isempty()){
            return ;
        }

        switch (cmd.rop) {

            case 0x00: this->screen->paintCache().fillRect(drect.x, drect.y, drect.cx, drect.cy, Qt::black);
                break;

            case 0x22: this->draw_memblt_op<Op_0x22>(drect, bitmap);
                break;

            case 0x33: this->draw_MemBlt(drect, bitmap, true, cmd.srcx + (drect.x - cmd.rect.x), cmd.srcy + (drect.y - cmd.rect.y));
                break;

            case 0x55:
                this->draw_memblt_op<Op_0x55>(drect, bitmap);
                break;

            case 0x66: this->draw_memblt_op<Op_0x66>(drect, bitmap);
                break;

            case 0x99:  this->draw_memblt_op<Op_0x99>(drect, bitmap);
                break;

            case 0xAA:  // nothing to change
                break;

            case 0xBB: this->draw_memblt_op<Op_0xBB>(drect, bitmap);
                break;

            case 0xCC: this->draw_MemBlt(drect, bitmap, false, cmd.srcx + (drect.x - cmd.rect.x), cmd.srcy + (drect.y - cmd.rect.y));
                break;

            case 0xEE: this->draw_memblt_op<Op_0xEE>(drect, bitmap);
                break;

            case 0x88: this->draw_memblt_op<Op_0x88>(drect, bitmap);
                break;

            case 0xFF: this->screen->paintCache().fillRect(drect.x, drect.y, drect.cx, drect.cy, Qt::white);
                break;

            default: LOG(LOG_WARNING, "DEFAULT: RDPMemBlt rop = %x", cmd.rop);
                break;
        }

        if (this->is_recording && !this->is_replaying) {
            this->graph_capture->draw(cmd, clip, bitmap);
            struct timeval time;
            gettimeofday(&time, nullptr);
            this->capture.get()->periodic_snapshot(time, this->mouse_data.x, this->mouse_data.y, false);
        }
    }


    void draw(const RDPMem3Blt & cmd, Rect clip, gdi::ColorCtx color_ctx, const Bitmap & bitmap) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }
        (void) color_ctx;
        //std::cout << "RDPMem3Blt " << std::hex << int(cmd.rop) << std::dec <<  std::endl;
        const Rect drect = clip.intersect(cmd.rect);
        if (drect.isempty()){
            return ;
        }

        switch (cmd.rop) {
            case 0xB8:
                {
                    const uint16_t mincx = std::min<int16_t>(bitmap.cx(), std::min<int16_t>(this->info.width  - drect.x, drect.cx));
                    const uint16_t mincy = std::min<int16_t>(bitmap.cy(), std::min<int16_t>(this->info.height - drect.y, drect.cy));

                    if (mincx <= 0 || mincy <= 0) {
                        return;
                    }
                    uint32_t fore_color24 = color_decode_opaquerect(cmd.fore_color, this->mod_bpp, this->mod_palette);
                    const QColor fore(this->u32_to_qcolor(fore_color24));
                    const uint8_t r(fore.red());
                    const uint8_t g(fore.green());
                    const uint8_t b(fore.blue());

                    int rowYCoord(drect.y + drect.cy-1);
                    const QImage::Format format(this->bpp_to_QFormat(bitmap.bpp(), true));

                    QImage dstBitmap(this->screen->getCache()->toImage().copy(drect.x, drect.y, mincx, mincy));
                    QImage srcBitmap(bitmap.data(), mincx, mincy, bitmap.line_size(), format);
                    srcBitmap = srcBitmap.convertToFormat(QImage::Format_RGB888);
                    dstBitmap = dstBitmap.convertToFormat(QImage::Format_RGB888);

                    const size_t rowsize(srcBitmap.bytesPerLine());
                    std::unique_ptr<uchar[]> data = std::make_unique<uchar[]>(rowsize);


                    for (size_t k = 1 ; k < drect.cy; k++) {

                        const uchar * srcData = srcBitmap.constScanLine(k);
                        const uchar * dstData = dstBitmap.constScanLine(mincy - k);

                        for (size_t x = 0; x < rowsize-2; x += 3) {
                            data[x  ] = ((dstData[x  ] ^ r) & srcData[x  ]) ^ r;
                            data[x+1] = ((dstData[x+1] ^ g) & srcData[x+1]) ^ g;
                            data[x+2] = ((dstData[x+2] ^ b) & srcData[x+2]) ^ b;
                        }

                        QImage image(data.get(), mincx, 1, srcBitmap.format());
                        if (image.depth() != this->info.bpp) {
                            image = image.convertToFormat(this->imageFormatRGB);
                        }
                        QRect trect(drect.x, rowYCoord, mincx, 1);
                        this->screen->paintCache().drawImage(trect, image);
                        rowYCoord--;
                    }
                }
            break;

            default: LOG(LOG_WARNING, "DEFAULT: RDPMem3Blt rop = %x", cmd.rop);
            break;
        }

        if (this->is_recording && !this->is_replaying) {
            this->graph_capture->draw(cmd, clip, gdi::ColorCtx(gdi::Depth::from_bpp(this->info.bpp), &this->mod_palette), bitmap);
            struct timeval time;
            gettimeofday(&time, nullptr);
            this->capture.get()->periodic_snapshot(time, this->mouse_data.x, this->mouse_data.y, false);
        }
    }


    void draw(const RDPDestBlt & cmd, Rect clip) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        const Rect drect = clip.intersect(this->info.width, this->info.height).intersect(cmd.rect);

        switch (cmd.rop) {
            case 0x00: // blackness
                this->screen->paintCache().fillRect(drect.x, drect.y, drect.cx, drect.cy, Qt::black);
                break;
            case 0x55:                                         // inversion
                this->draw_RDPScrBlt(drect.x, drect.y, drect, true);
                break;
            case 0xAA: // change nothing
                break;
            case 0xFF: // whiteness
                this->screen->paintCache().fillRect(drect.x, drect.y, drect.cx, drect.cy, Qt::white);
                break;
            default: LOG(LOG_WARNING, "DEFAULT: RDPDestBlt rop = %x", cmd.rop);
                break;
        }

        if (this->is_recording && !this->is_replaying) {
            this->graph_capture->draw(cmd, clip);
            struct timeval time;
            gettimeofday(&time, nullptr);
            this->capture.get()->periodic_snapshot(time, this->mouse_data.x, this->mouse_data.y, false);
        }
    }

    void draw(const RDPMultiDstBlt & cmd, Rect clip) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        LOG(LOG_WARNING, "DEFAULT: RDPMultiDstBlt");
    }

    void draw(const RDPMultiOpaqueRect & cmd, Rect clip, gdi::ColorCtx color_ctx) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }
        (void) color_ctx;
        LOG(LOG_WARNING, "DEFAULT: RDPMultiOpaqueRect");
    }

    void draw(const RDP::RDPMultiPatBlt & cmd, Rect clip, gdi::ColorCtx color_ctx) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }
        (void) color_ctx;
        LOG(LOG_WARNING, "DEFAULT: RDPMultiPatBlt");
    }

    void draw(const RDP::RDPMultiScrBlt & cmd, Rect clip) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }

        LOG(LOG_WARNING, "DEFAULT: RDPMultiScrBlt");
    }

    void draw(const RDPGlyphIndex & cmd, Rect clip, gdi::ColorCtx color_ctx, const GlyphCache & gly_cache) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }
        (void) color_ctx;
        Rect screen_rect = clip.intersect(this->info.width, this->info.height);
        if (screen_rect.isempty()){
            return ;
        }

        Rect const clipped_glyph_fragment_rect = cmd.bk.intersect(screen_rect);
        if (clipped_glyph_fragment_rect.isempty()) {
            return;
        }
        //std::cout << "RDPGlyphIndex " << std::endl;

        // set a background color
        {
            /*Rect ajusted = cmd.f_op_redundant ? cmd.bk : cmd.op;
            if ((ajusted.cx > 1) && (ajusted.cy > 1)) {
                ajusted.cy--;
                ajusted.intersect(screen_rect);
                this->screen->paintCache().fillRect(ajusted.x, ajusted.y, ajusted.cx, ajusted.cy, this->u32_to_qcolor(color_decode_opaquerect(cmd.fore_color, this->info.bpp, this->mod_palette)));
            }*/
        }

        bool has_delta_bytes = (!cmd.ui_charinc && !(cmd.fl_accel & 0x20));

        const QColor color = this->u32_to_qcolor(color_decode_opaquerect(cmd.back_color, this->info.bpp, this->mod_palette));
        const int16_t offset_y = /*cmd.bk.cy - (*/cmd.glyph_y - cmd.bk.y/* + 1)*/;
        const int16_t offset_x = cmd.glyph_x - cmd.bk.x;

        uint16_t draw_pos = 0;

        InStream variable_bytes(cmd.data, cmd.data_len);

        //uint8_t const * fragment_begin_position = variable_bytes.get_current();

        while (variable_bytes.in_remain()) {
            uint8_t data = variable_bytes.in_uint8();

            if (data <= 0xFD) {
                FontChar const & fc = gly_cache.glyphs[cmd.cache_id][data].font_item;
                if (!fc)
                {
                    LOG( LOG_INFO
                        , "RDPDrawable::draw_VariableBytes: Unknown glyph, cacheId=%u cacheIndex=%u"
                        , cmd.cache_id, data);
                    REDASSERT(fc);
                }

                if (has_delta_bytes)
                {
                    data = variable_bytes.in_uint8();
                    if (data == 0x80)
                    {
                        draw_pos += variable_bytes.in_uint16_le();
                    }
                    else
                    {
                        draw_pos += data;
                    }
                }

                if (fc)
                {
                    const int16_t x = draw_pos + cmd.bk.x + offset_x;
                    const int16_t y = offset_y + cmd.bk.y;
                    if (Rect(0,0,0,0) != clip.intersect(Rect(x, y, fc.incby, fc.height))){

                        const uint8_t * fc_data            = fc.data.get();
                        for (int yy = 0 ; yy < fc.height; yy++)
                        {
                            uint8_t   fc_bit_mask        = 128;
                            for (int xx = 0 ; xx < fc.width; xx++)
                            {
                                if (!fc_bit_mask)
                                {
                                    fc_data++;
                                    fc_bit_mask = 128;
                                }
                                if (clip.contains_pt(x + fc.offset + xx, y + fc.baseline + yy)
                                && (fc_bit_mask & *fc_data))
                                {
                                    this->screen->paintCache().fillRect(x + fc.offset + xx, y + fc.baseline + yy, 1, 1, color);
                                }
                                fc_bit_mask >>= 1;
                            }
                            fc_data++;
                        }
                    }
                }
            } else {
                LOG(LOG_WARNING, "DEFAULT: RDPGlyphIndex glyph_cache");
            }
        }
        //this->draw_VariableBytes(cmd.data, cmd.data_len, has_delta_bytes,
            //draw_pos, offset_y, color, cmd.bk.x + offset_x, cmd.bk.y,
            //clipped_glyph_fragment_rect, cmd.cache_id, gly_cache);
        if (this->is_recording && !this->is_replaying) {
            this->graph_capture->draw(cmd, clip, gdi::ColorCtx(gdi::Depth::from_bpp(this->info.bpp), &this->mod_palette), gly_cache);
            struct timeval time;
            gettimeofday(&time, nullptr);
            this->capture.get()->periodic_snapshot(time, this->mouse_data.x, this->mouse_data.y, false);
        }
    }

    void draw(const RDPPolygonSC & cmd, Rect clip, gdi::ColorCtx color_ctx) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }
        (void) color_ctx;
        LOG(LOG_WARNING, "DEFAULT: RDPPolygonSC");

        /*RDPPolygonSC new_cmd24 = cmd;
        new_cmd24.BrushColor  = color_decode_opaquerect(cmd.BrushColor,  this->mod_bpp, this->mod_palette);*/
        //this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDPPolygonCB & cmd, Rect clip, gdi::ColorCtx color_ctx) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }
        (void) color_ctx;
        LOG(LOG_WARNING, "DEFAULT: RDPPolygonCB");

        /*RDPPolygonCB new_cmd24 = cmd;
        new_cmd24.foreColor  = color_decode_opaquerect(cmd.foreColor,  this->mod_bpp, this->mod_palette);
        new_cmd24.backColor  = color_decode_opaquerect(cmd.backColor,  this->mod_bpp, this->mod_palette);*/
        //this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDPPolyline & cmd, Rect clip, gdi::ColorCtx color_ctx) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }
        (void) color_ctx;
        LOG(LOG_WARNING, "DEFAULT: RDPPolyline");
        /*RDPPolyline new_cmd24 = cmd;
        new_cmd24.PenColor  = color_decode_opaquerect(cmd.PenColor,  this->mod_bpp, this->mod_palette);*/
        //this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDPEllipseSC & cmd, Rect clip, gdi::ColorCtx color_ctx) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }
        (void) color_ctx;
        LOG(LOG_WARNING, "DEFAULT: RDPEllipseSC");

        /*RDPEllipseSC new_cmd24 = cmd;
        new_cmd24.color = color_decode_opaquerect(cmd.color, this->mod_bpp, this->mod_palette);*/
        //this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDPEllipseCB & cmd, Rect clip, gdi::ColorCtx color_ctx) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            cmd.log(LOG_INFO, clip);
            LOG(LOG_INFO, "========================================\n");
        }
        (void) color_ctx;
        LOG(LOG_WARNING, "DEFAULT: RDPEllipseCB");
    /*
        RDPEllipseCB new_cmd24 = cmd;
        new_cmd24.fore_color = color_decode_opaquerect(cmd.fore_color, this->mod_bpp, this->mod_palette);
        new_cmd24.back_color = color_decode_opaquerect(cmd.back_color, this->mod_bpp, this->mod_palette);*/
        //this->gd.draw(new_cmd24, clip);
    }

    void draw(const RDP::FrameMarker & order) override {
        if (bool(this->verbose & RDPVerbose::graphics)) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            order.log(LOG_INFO);
            LOG(LOG_INFO, "========================================\n");
        }

        if (this->is_recording && !this->is_replaying) {
            this->graph_capture->draw(order);
            struct timeval time;
            gettimeofday(&time, nullptr);
            this->capture.get()->periodic_snapshot(time, this->mouse_data.x, this->mouse_data.y, false);
        }

        LOG(LOG_INFO, "DEFAULT: FrameMarker");
    }







    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //------------------------
    //      CONTROLLERS
    //------------------------

    void mousePressEvent(QMouseEvent *e, int screen_shift) override {
        if (this->mod != nullptr) {
            int flag(0);
            switch (e->button()) {
                case Qt::LeftButton:  flag = MOUSE_FLAG_BUTTON1; break;
                case Qt::RightButton: flag = MOUSE_FLAG_BUTTON2; break;
                case Qt::MidButton:   flag = MOUSE_FLAG_BUTTON4; break;
                case Qt::XButton1:
                case Qt::XButton2:
                case Qt::NoButton:
                case Qt::MouseButtonMask:

                default: break;
            }

            this->mod->rdp_input_mouse(flag | MOUSE_FLAG_DOWN, e->x() + screen_shift, e->y(), &(this->keymap));
        }
    }

    void mouseReleaseEvent(QMouseEvent *e, int screen_shift) override {
        if (this->mod != nullptr) {
            int flag(0);
            switch (e->button()) {

                case Qt::LeftButton:  flag = MOUSE_FLAG_BUTTON1; break;
                case Qt::RightButton: flag = MOUSE_FLAG_BUTTON2; break;
                case Qt::MidButton:   flag = MOUSE_FLAG_BUTTON4; break;
                case Qt::XButton1:
                case Qt::XButton2:
                case Qt::NoButton:
                case Qt::MouseButtonMask:

                default: break;
            }

            this->mod->rdp_input_mouse(flag, e->x() + screen_shift, e->y(), &(this->keymap));
        }
    }

    void keyPressEvent(QKeyEvent *e) override {
        this->qtRDPKeymap.keyEvent(0     , e);
        if (this->qtRDPKeymap.scanCode != 0) {
            this->send_rdp_scanCode(this->qtRDPKeymap.scanCode, this->qtRDPKeymap.flag);
        }
    }

    void keyReleaseEvent(QKeyEvent *e) override {
        this->qtRDPKeymap.keyEvent(KBD_FLAG_UP, e);
        if (this->qtRDPKeymap.scanCode != 0) {
            this->send_rdp_scanCode(this->qtRDPKeymap.scanCode, this->qtRDPKeymap.flag);
        }
    }

    void wheelEvent(QWheelEvent *e) override {
        int flag(MOUSE_FLAG_HWHEEL);
        if (e->delta() < 0) {
            flag = flag | MOUSE_FLAG_WHEEL_NEGATIVE;
        }
        if (this->mod != nullptr) {
            //this->mod->rdp_input_mouse(flag, e->x(), e->y(), &(this->keymap));
        }
    }

    bool eventFilter(QObject *, QEvent *e, int screen_shift) override {
        if (e->type() == QEvent::MouseMove)
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(e);
            int x = mouseEvent->x() + screen_shift;
            int y = mouseEvent->y();

            if (x < 0) {
                x = 0;
            }
            if (y < 0) {
                y = 0;
            }

            if (y > this->info.height) {
                this->screen->mouse_out = true;
            } else if (this->screen->mouse_out) {
                this->screen->update_current_cursor();
                this->screen->mouse_out = false;
            }

            if (this->mod != nullptr) {
                this->mouse_data.x = x;
                this->mouse_data.y = y;
                this->mod->rdp_input_mouse(MOUSE_FLAG_MOVE, x, y, &(this->keymap));
            }
        }
        return false;
    }

    bool connexionReleased() override {
        this->form->setCursor(Qt::WaitCursor);
        this->user_name     = this->form->get_userNameField();
        this->target_IP     = this->form->get_IPField();
        this->user_password = this->form->get_PWDField();
        this->port          = this->form->get_portField();

        bool res(false);
        if (!this->target_IP.empty()){
            res = this->connect();
        }
        this->form->setCursor(Qt::ArrowCursor);

        return res;
    }

    void RefreshPressed() override {
        Rect rect(0, 0, this->info.width, this->info.height);
        this->mod->rdp_input_invalidate(rect);
    }

    virtual void CtrlAltDelPressed() override {
        LOG(LOG_WARNING, "CtrlAltDel not implemented yet.");
    }

    virtual void CtrlAltDelReleased() override {}

    void disconnexionReleased() override{
        this->is_replaying = false;
        this->dropScreen();
        this->disconnect("");
        this->cache = nullptr;
        this->trans_cache = nullptr;
        this->capture = nullptr;
        this->graph_capture = nullptr;
    }

    void setMainScreenOnTopRelease() override {
        this->screen->activateWindow();
    }

    void send_rdp_scanCode(int keyCode, int flag) {
        Keymap2::DecodedKeys decoded_keys = this->keymap.event(flag, keyCode, this->ctrl_alt_delete);
        switch (decoded_keys.count)
        {
        case 2:
            if (this->decoded_data.has_room(sizeof(uint32_t))) {
                this->decoded_data.out_uint32_le(decoded_keys.uchars[0]);
            }
            if (this->decoded_data.has_room(sizeof(uint32_t))) {
                this->decoded_data.out_uint32_le(decoded_keys.uchars[1]);
            }
            break;
        case 1:
            if (this->decoded_data.has_room(sizeof(uint32_t))) {
                this->decoded_data.out_uint32_le(decoded_keys.uchars[0]);
            }
            break;
        default:
        case 0:
            break;
        }
        if (this->mod != nullptr) {
            this->mod->rdp_input_scancode(keyCode, 0, flag, this->_timer, &(this->keymap));
        }
    }

    void closeFromScreen() override {

        if (this->screen != nullptr) {
            this->screen->disconnection();
            this->screen->close();
            this->screen = nullptr;
        }

        this->capture = nullptr;
        this->graph_capture = nullptr;

        if (this->form != nullptr && this->connected) {
            this->form->close();
        }
    }

    void dropScreen() override {
        if (this->screen != nullptr) {
            this->screen->disconnection();
            this->screen->close();
            this->screen = nullptr;
        }
    }

    void replay(std::string const & movie_path_) override {
        if (movie_path_.empty()) {
            return;
        }
        auto const last_delimiter_it = std::find(movie_path_.rbegin(), movie_path_.rend(), '/');
        std::string const movie_path = (last_delimiter_it == movie_path_.rend())
        ? movie_path_
        : movie_path_.substr(movie_path_.size() - (last_delimiter_it - movie_path_.rbegin()));

        this->is_replaying = true;
        //this->setScreenDimension();
        this->cache_replay = new QPixmap(this->info.width, this->info.height);
        this->trans_cache = new QPixmap(this->info.width, this->info.height);
        this->trans_cache->fill(Qt::transparent);
        this->screen = new Screen_Qt(this, this->cache_replay, movie_path, this->trans_cache);

        this->connected = true;
        this->form->hide();
        this->screen->show();

        this->load_replay_mod(movie_path);
    }

    virtual bool connect() {
        if (this->mod_qt->connect()) {
            this->cache = new QPixmap(this->info.width, this->info.height);
            this->trans_cache = new QPixmap(this->info.width, this->info.height);
            this->trans_cache->fill(Qt::transparent);
            this->screen = new Screen_Qt(this, this->cache, this->trans_cache);

            this->is_replaying = false;
            this->connected = true;



            if (this->is_recording && !this->is_replaying) {
                Inifile ini;
                    ini.set<cfg::video::capture_flags>(CaptureFlags::wrm | CaptureFlags::png);
                    ini.set<cfg::video::png_limit>(0);
                    ini.set<cfg::video::disable_keyboard_log>(KeyboardLogFlags::none);
                    ini.set<cfg::session_log::enable_session_log>(0);
                    ini.set<cfg::session_log::keyboard_input_masking_level>(KeyboardInputMaskingLevel::unmasked);
                    ini.set<cfg::context::pattern_kill>("");
                    ini.set<cfg::context::pattern_notify>("");
                    ini.set<cfg::debug::capture>(0xfffffff);
                    ini.set<cfg::video::capture_groupid>(1);
                    ini.set<cfg::video::record_tmp_path>(this->REPLAY_DIR);
                    ini.set<cfg::video::record_path>(this->REPLAY_DIR);
                    ini.set<cfg::video::hash_path>(this->REPLAY_DIR);
                    time_t now;
                    time(&now);
                    std::string data(ctime(&now));
                    std::string data_cut(data.c_str(), data.size()-1);
                    std::string name("-Replay");
                    std::string movie_name(data_cut+name);
                    ini.set<cfg::globals::movie_path>(movie_name.c_str());
                    ini.set<cfg::globals::trace_type>(TraceType::localfile);
                    ini.set<cfg::video::wrm_compression_algorithm>(WrmCompressionAlgorithm::no_compression);
                    ini.set<cfg::video::frame_interval>(std::chrono::duration<unsigned, std::ratio<1, 100>>(6));

                LCGRandom gen(0);
                CryptoContext cctx;
                DummyAuthentifier * authentifier = nullptr;
                struct timeval time;
                gettimeofday(&time, nullptr);
                PngParams png_params = {0, 0, std::chrono::milliseconds{60}, 100, 0, true, authentifier, ini.get<cfg::video::record_tmp_path>().c_str(), "", 1};
                FlvParams flv_params = flv_params_from_ini(this->info.width, this->info.height, ini);
                OcrParams ocr_params = { ini.get<cfg::ocr::version>(),
                                        static_cast<ocr::locale::LocaleId::type_id>(ini.get<cfg::ocr::locale>()),
                                        ini.get<cfg::ocr::on_title_bar_only>(),
                                        ini.get<cfg::ocr::max_unrecog_char_rate>(),
                                        ini.get<cfg::ocr::interval>()
                                    };


                std::string record_path = this->REPLAY_DIR.c_str() + std::string("/");

                Fstat fstat;

                WrmParams wrmParams(
                    this->info.bpp
                    , TraceType::localfile
                    , cctx
                    , gen
                    , fstat
                    , record_path.c_str()
                    , ini.get<cfg::video::hash_path>().c_str()
                    , movie_name.c_str()
                    , ini.get<cfg::video::capture_groupid>()
                    , std::chrono::duration<unsigned int, std::ratio<1l, 100l> >{60}
                    , std::chrono::seconds{1}
                    , WrmCompressionAlgorithm::no_compression
                    , 0
                );

                PatternCheckerParams patternCheckerParams;
                SequencedVideoParams sequenced_video_params;
                FullVideoParams full_video_params;
                MetaParams meta_params;
                KbdLogParams kbd_log_params;

                this->capture = std::make_unique<Capture>( true, wrmParams
                                            , false, png_params
                                            , false, patternCheckerParams
                                            , false, ocr_params
                                            , false, sequenced_video_params
                                            , false, full_video_params
                                            , false, meta_params
                                            , false, kbd_log_params
                                            , ""
                                            , time
                                            , this->info.width
                                            , this->info.height
                                            , this->info.bpp
                                            , this->info.bpp
                                            , ini.get<cfg::video::record_tmp_path>().c_str()
                                            , ini.get<cfg::video::record_tmp_path>().c_str()
                                            , 1
                                            , flv_params
                                            , false
                                            , authentifier
                                            , nullptr
                                            , ""
                                            , ""
                                            , 0xfffffff
                                            , false
                                            , false
                                            , std::chrono::duration<long int>{60}
                                            , false
                                            , false
                                            , false
                                            , false
                                            , false
                                            , false
                                            );

                this->graph_capture = this->capture.get()->get_graphic_api();
            }

            if (this->mod_qt->listen()) {
                this->form->hide();
                this->screen->show();
                return true;
            }
        }

        return false;
    }

    void disconnect(std::string const & error) override {

        if (this->mod_qt != nullptr) {
            this->mod_qt->disconnect();
        }

        this->form->set_IPField(this->target_IP);
        this->form->set_portField(this->port);
        this->form->set_PWDField(this->user_password);
        this->form->set_userNameField(this->user_name);
        this->form->set_ErrorMsg(error);
        this->form->show();

        this->connected = false;
    }

    virtual const CHANNELS::ChannelDefArray & get_channel_list(void) const override {
        return this->cl;
    }



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //--------------------------------
    //    SOCKET EVENTS FUNCTIONS
    //--------------------------------

    virtual void callback() override {

        if (this->mod != nullptr && this->cache != nullptr) {
            try {
                this->mod->draw_event(time(nullptr), *(this));
            } catch (const Error &) {
                this->dropScreen();
                const std::string errorMsg("Error: connexion to [" + this->target_IP +  "] is closed.");
                LOG(LOG_INFO, "%s", errorMsg.c_str());
                std::string labelErrorMsg("<font color='Red'>"+errorMsg+"</font>");

                this->disconnect(labelErrorMsg);
            }
        }
    }


};
