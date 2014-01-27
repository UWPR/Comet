sftp eng_jk@web.sourceforge.net
cd  /home/project-web/comet-ms/htdocs/

rsync -avP -e ssh . eng_jk@web.sourceforge.net:/home/project-web/comet-ms/htdocs/
