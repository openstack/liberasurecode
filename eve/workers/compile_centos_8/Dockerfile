FROM centos:8.2.2004

ENV HOME_BUILDBOT /var/lib/buildbot

# Enable EPEL
RUN dnf install -y epel-release yum-utils glibc-langpack-en dnf-plugins-core

# Activate devel packages
RUN dnf config-manager --set-enabled PowerTools

# Mix of dependency of buildbot & build.
RUN dnf install -y \
        autoconf \
        automake \
        gcc \
        git \
        libtool \
        make \
        python2-pip \
        sudo \
        zlib-devel

RUN git config --global credential.helper store

# Add user eve (with sudo)
RUN adduser -u 1042 eve \
    && groupadd sudo \
    && usermod -a -G sudo eve \
    && echo 'eve ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers.d/eve

# Eve configuration
USER eve

RUN mkdir -p /home/eve/workspace \
    && mkdir -p /home/eve/.ssh/ \
    && /bin/echo -e "Host bitbucket.org\n\tStrictHostKeyChecking no\n" >> /home/eve/.ssh/config

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

ENV BUILD_DISTRO=centos8

# Run buildbot-worker on startup
ARG BUILDBOT_VERSION=0.9.12
RUN sudo pip2 install buildbot-worker==$BUILDBOT_VERSION

WORKDIR /home/eve/workspace
CMD buildbot-worker create-worker . "$BUILDMASTER:$BUILDMASTER_PORT" "$WORKERNAME" "$WORKERPASS" \
    && buildbot-worker start --nodaemon
