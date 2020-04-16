FROM ubuntu:xenial

ARG BUILDBOT_VERSION

#
# Set environment
#

RUN apt-get update \
 && apt-get install -y apt-utils locales sudo \
 && locale-gen en_US.UTF-8 \
 && echo "LANG=en_US.UTF-8\nLC_ALL=en_US.UTF-8" > /etc/default/locale \
 && rm -rf /var/lib/apt/lists/*

ENV LANG=en_US.UTF-8 \
    DEBIAN_FRONTEND=noninteractive \
    TERM=xterm

#
# Creation of the user and group scality
#

RUN groupadd --gid 1042 scality \
# Must be the same gid of docker group than the one on
# eve-prod-dockerhost*
 && groupadd --gid ${DOCKER_GID:-999} docker \
 && useradd \
      --uid 1042 \
      --gid scality \
      --groups docker \
      --home /home/scality \
      --create-home \
      --shell /bin/bash \
      scality \
 && adduser scality sudo \
 && sed -ri 's/(%sudo.*)ALL$/\1NOPASSWD:ALL/' /etc/sudoers

#
# Configuration
#

COPY config/ssh-config /home/scality/.ssh/config
RUN chown -R scality: /home/scality/.ssh && \
    chmod 0600 /home/scality/.ssh/config

#
# Add apt-transport-https (necessary for git-lfs repository)
#

RUN apt-get update \
    && apt-get install -y \
      apt-transport-https \
      wget \
    && rm -rf /var/lib/apt/lists/*

#
# Add git-lfs repository
#

COPY config/github_git-lfs.list /etc/apt/sources.list.d
RUN wget -q -O - https://packagecloud.io/github/git-lfs/gpgkey \
    | apt-key add -

#
# Add git-core repository
#

COPY config/git_core.list /etc/apt/sources.list.d
COPY config/git_core.pubkey /tmp/
RUN apt-key add /tmp/git_core.pubkey \
    && rm -f /tmp/git_core.pubkey

#
# Install needed packages
#

COPY requirements/packages.list /tmp
RUN apt-get update \
    && cat /tmp/packages.list \
     | xargs apt-get install -y \
    && rm -rf /var/lib/apt/lists/* \
    && rm -f /tmp/packages.list

#
# Install python buildbot-worker and buildenv
#

COPY requirements/python2.txt /tmp/requirements-py2.txt
COPY requirements/python3.txt /tmp/requirements-py3.txt
COPY wheels/*.whl /usr/share/python-wheels

RUN pip2 install --no-cache-dir \
      --disable-pip-version-check \
      --require-hashes \
      -r /tmp/requirements-py2.txt \
 && pip3 install --no-cache-dir \
      --disable-pip-version-check \
      --require-hashes \
      -r /tmp/requirements-py3.txt \
 && pip3 install --no-cache-dir \
      --no-index \
      --find-links /usr/share/python-wheels \
      --no-deps \
      ring-buildenv \
 && rm -f /tmp/requirements-py*.txt

#
# Configuration of git-lfs
# (note: url conf is overriden by Eve 1.3.1+)
#

RUN git lfs install --system --skip-smudge \
 && git config --system lfs.url \
      "http://bitbucket.org/scality/ring.git/info/lfs"

#
# Run buildbot as scality user
#

RUN install -d -o scality /home/scality/eve

USER scality

WORKDIR /home/scality/eve

VOLUME ["/home/scality/eve"]

#
# Copy entrypoint and define start command
#

COPY entrypoint.sh /
ENTRYPOINT ["/entrypoint.sh"]

CMD ["start", "--nodaemon"]
