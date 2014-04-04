l2elog
======

This application forwards log messages that would normally go to Linux's
syslog and klog daemons through Erlang's Lager logger. While this may seem
completely backwards, when using the Nerves framework, Erlang takes a central
role and much of the standard Linux userspace is either not needed or minimal.
In this environment, most log messages originate in Erlang, but not all. This
utility forwards the remainder of those messages so that messages from the
kernel and any C programs aren't lost.

This application depends on Lager, but it's assumed that other applications on
the platform also use Lager. See the Lager documentation for how to configure
Lager to save messages to the desired locations.
