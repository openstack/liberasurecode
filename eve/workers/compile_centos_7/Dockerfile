FROM centos:7.7.1908

ENV HOME_BUILDBOT /var/lib/buildbot

# Enable EPEL
RUN yum install -y epel-release

# Mix of dependency of buildbot & build.
RUN yum install -y \
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

ENV BUILD_DISTRO=centos7

# Run buildbot-worker on startup
ARG BUILDBOT_VERSION=0.9.12
RUN sudo pip install buildbot-worker==$BUILDBOT_VERSION

WORKDIR /home/eve/workspace
CMD buildbot-worker create-worker . "$BUILDMASTER:$BUILDMASTER_PORT" "$WORKERNAME" "$WORKERPASS" \
    && buildbot-worker start --nodaemon
