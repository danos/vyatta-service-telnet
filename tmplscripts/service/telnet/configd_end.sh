#!/opt/vyatta/bin/cliexec
sed -i -e '/^# Unix98 PTY slaves/,/#.../d' /etc/securetty
if [[ ${COMMIT_ACTION} != "DELETE" ]]; then
    /opt/vyatta/sbin/vyatta_update_telnet --vrf=$VAR(../../../routing-instance/@) --operation=enable --port=$VAR(port/@) --address=$VAR(listen-address/@)
fi
