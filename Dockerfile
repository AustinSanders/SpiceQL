FROM public.ecr.aws/lambda/python:3.9

RUN echo "sslverify=false" >> /etc/yum.conf

RUN yum groupinstall -y 'Development Tools' 	
RUN yum install -y nfs-utils amazon-efs-utils

# Copy function code
COPY aws/lambda_function.py ${LAMBDA_TASK_ROOT}

# Install miniconda
ENV CONDA_DIR ~/conda

RUN curl -L -O https://github.com/conda-forge/miniforge/releases/latest/download/Mambaforge-Linux-x86_64.sh --insecure
RUN bash Mambaforge-Linux-x86_64.sh -b -p $CONDA_DIR

# Install the function's dependencies using file requirements.txt
# from your project folder.

RUN rm -rf /var/lang/
COPY environment.yml  .
RUN $CONDA_DIR/bin/mamba env create --file environment.yml --prefix /var/lang 

ENV PATH=/var/lang/bin:$PATH

RUN mkdir /repo 
COPY . /repo/

RUN ls -lrt /var/lang/include/ && cd /repo && \
    mkdir -p build && \
    cd build && cmake .. -DCMAKE_INSTALL_PREFIX=/var/lang/ -DSPICEQL_BUILD_DOCS=NO -DSPICEQL_BUILD_TESTS=NO && \
    make install
RUN cd /repo/build/bindings/python/ && python setup.py install --prefix=/var/lang
RUN chmod 755 -R /repo
COPY aws/entrypoint.sh entrypoint.sh

ENV SSPICE_DEBUG=TRUE
ENV SPICEROOT=/mnt/isis_data/isis_data
ENV SPICEQL_LOG_LEVEL=TRACE

ENTRYPOINT [ "bash", "entrypoint.sh" ]

# Set the CMD to your handler (could also be done as a parameter override outside of the Dockerfile)
CMD ["lambda_function.lambda_handler"]

